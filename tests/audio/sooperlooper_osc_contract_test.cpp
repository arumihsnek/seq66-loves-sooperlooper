/*
 *  Headless OSC contract tests for the Seq66 -> SooperLooper adapter.
 */

#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <lo/lo.h>

#include "audio/audio_clip.hpp"
#include "audio/sooperlooper_client.hpp"

namespace
{

struct captured_message
{
    std::string path;
    std::string types;
    std::vector<std::string> strings;
    std::vector<int> integers;
    std::vector<float> floats;
};

class capture_state
{

private:

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::vector<captured_message> m_messages;

public:

    void push (captured_message message)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_messages.push_back(std::move(message));
        }
        m_condition.notify_all();
    }

    std::size_t size () const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_messages.size();
    }

    bool wait_for_size
    (
        std::size_t expected,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)
    )
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_condition.wait_for
        (
            lock, timeout,
            [this, expected] { return m_messages.size() >= expected; }
        );
    }

    captured_message at (std::size_t index) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_messages.at(index);
    }

    bool contains_control
    (
        std::size_t first,
        const std::string & path,
        const std::string & control,
        float expected,
        float epsilon = 1.0e-5f
    ) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (std::size_t index = first; index < m_messages.size(); ++index)
        {
            const auto & message = m_messages[index];
            if
            (
                message.path == path && message.types == "sf" &&
                message.strings.size() == 1 &&
                message.strings.front() == control &&
                message.floats.size() == 1 &&
                std::fabs(message.floats.front() - expected) <= epsilon
            )
            {
                return true;
            }
        }
        return false;
    }
};

void
osc_error (int, const char *, const char *)
{
    // An unexpected liblo error will normally make a send/assert fail.
}

int
capture_handler
(
    const char * path,
    const char * types,
    lo_arg ** arguments,
    int argument_count,
    lo_message,
    void * user_data
)
{
    captured_message message;
    message.path = path ? path : "";
    message.types = types ? types : "";

    for (int index = 0; index < argument_count; ++index)
    {
        switch (types[index])
        {
        case 's':
            message.strings.emplace_back(&arguments[index]->s);
            break;

        case 'i':
            message.integers.push_back(arguments[index]->i);
            break;

        case 'f':
            message.floats.push_back(arguments[index]->f);
            break;

        default:
            assert(false && "Unexpected OSC type in contract test");
            break;
        }
    }

    static_cast<capture_state *>(user_data)->push(std::move(message));
    return 0;
}

bool
near (float left, float right, float epsilon = 1.0e-5f)
{
    return std::fabs(left - right) <= epsilon;
}

}           // namespace anonymous

int
main ()
{
    constexpr const char * port { "19951" };
    lo_server_thread server { lo_server_thread_new(port, osc_error) };
    assert(server != nullptr);

    capture_state state;
    lo_server_thread_add_method
    (
        server, nullptr, nullptr, capture_handler, &state
    );
    assert(lo_server_thread_start(server) == 0);

    seq66::sooperlooper_client client
    (
        "osc.udp://127.0.0.1:19951/"
    );
    assert(client.ready());

    std::size_t count { state.size() };

    assert(client.hit(2, seq66::sooperlooper_command::record));
    assert(state.wait_for_size(++count));
    auto message = state.at(count - 1);
    assert(message.path == "/sl/2/hit");
    assert(message.types == "s");
    assert(message.strings.size() == 1);
    assert(message.strings.front() == "record");

    assert(client.set_loop_control(2, "wet", 0.5f));
    assert(state.wait_for_size(++count));
    message = state.at(count - 1);
    assert(message.path == "/sl/2/set");
    assert(message.types == "sf");
    assert(message.strings.front() == "wet");
    assert(near(message.floats.front(), 0.5f));

    assert(client.set_global_control("tempo", 123.0f));
    assert(state.wait_for_size(++count));
    message = state.at(count - 1);
    assert(message.path == "/set");
    assert(message.types == "sf");
    assert(message.strings.front() == "tempo");
    assert(near(message.floats.front(), 123.0f));

    assert(client.add_loop(2, 12.0f));
    assert(state.wait_for_size(++count));
    message = state.at(count - 1);
    assert(message.path == "/loop_add");
    assert(message.types == "if");
    assert(message.integers.front() == 2);
    assert(near(message.floats.front(), 12.0f));

    assert(client.delete_last_loop());
    assert(state.wait_for_size(++count));
    message = state.at(count - 1);
    assert(message.path == "/loop_del");
    assert(message.types == "i");
    assert(message.integers.front() == -1);

    const std::size_t before_invalid { state.size() };
    assert(! client.hit(-2, seq66::sooperlooper_command::mute));
    assert
    (
        ! client.set_loop_control
        (
            0, "wet", std::numeric_limits<float>::quiet_NaN()
        )
    );
    assert(! client.add_loop(0, 10.0f));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(state.size() == before_invalid);

    seq66::audio_timing timing;
    timing.bars = 4;
    timing.beats_per_bar = 4;
    timing.beat_width = 4;
    timing.recorded_bpm = 120.0;

    seq66::audio_clip clip("elastic-test", 2);
    assert(clip.timing(timing));
    clip.runtime_loop_index(3);
    clip.sync_mode(seq66::audio_sync_mode::elastic);
    assert(clip.pitch_shift(7.0));

    const std::size_t policy_first { state.size() };
    assert(client.apply_sync_policy(clip, 60.0));

    /*
     * Global tempo + pitch/quantize/round + six elastic policy controls.
     */

    assert(state.wait_for_size(policy_first + 10));
    assert(state.contains_control(policy_first, "/set", "tempo", 60.0f));
    assert
    (
        state.contains_control
        (
            policy_first, "/sl/3/set", "pitch_shift", 7.0f
        )
    );
    assert
    (
        state.contains_control
        (
            policy_first, "/sl/3/set", "tempo_stretch", 1.0f
        )
    );
    assert
    (
        state.contains_control
        (
            policy_first, "/sl/3/set", "stretch_ratio", 2.0f
        )
    );
    assert
    (
        state.contains_control
        (
            policy_first, "/sl/3/set", "use_rate", 0.0f
        )
    );

    lo_server_thread_stop(server);
    lo_server_thread_free(server);
    return 0;
}
