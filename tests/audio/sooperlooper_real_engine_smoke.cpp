/*
 *  Real-engine smoke test for a headless SooperLooper instance.
 *
 *  The fixture must start SooperLooper with zero loops on port 19953 and a
 *  JACK-compatible dummy/native server before running this binary.
 */

#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <lo/lo.h>

namespace
{

class probe_state
{

private:

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    unsigned m_ping_serial { 0 };
    unsigned m_value_serial { 0 };
    std::string m_engine_url;
    std::string m_version;
    int m_loop_count { -1 };
    int m_loop_index { -1 };
    std::string m_control;
    float m_value { 0.0f };

public:

    void ping (const std::string & url, const std::string & version, int loops)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_engine_url = url;
            m_version = version;
            m_loop_count = loops;
            ++m_ping_serial;
        }
        m_condition.notify_all();
    }

    void value (int loop, const std::string & control, float value)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_loop_index = loop;
            m_control = control;
            m_value = value;
            ++m_value_serial;
        }
        m_condition.notify_all();
    }

    unsigned ping_serial () const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_ping_serial;
    }

    unsigned value_serial () const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_value_serial;
    }

    bool wait_ping_after
    (
        unsigned previous,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(500)
    )
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_condition.wait_for
        (
            lock, timeout,
            [this, previous] { return m_ping_serial > previous; }
        );
    }

    bool wait_value_after
    (
        unsigned previous,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(500)
    )
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_condition.wait_for
        (
            lock, timeout,
            [this, previous] { return m_value_serial > previous; }
        );
    }

    int loop_count () const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_loop_count;
    }

    std::string version () const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_version;
    }

    int loop_index () const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_loop_index;
    }

    std::string control () const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_control;
    }

    float value () const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_value;
    }
};

void
osc_error (int, const char *, const char *)
{
    // Assertions and fixture logs provide the actionable diagnostics.
}

int
ping_handler
(
    const char *, const char *, lo_arg ** arguments, int,
    lo_message, void * user_data
)
{
    static_cast<probe_state *>(user_data)->ping
    (
        &arguments[0]->s, &arguments[1]->s, arguments[2]->i
    );
    return 0;
}

int
value_handler
(
    const char *, const char *, lo_arg ** arguments, int,
    lo_message, void * user_data
)
{
    static_cast<probe_state *>(user_data)->value
    (
        arguments[0]->i, &arguments[1]->s, arguments[2]->f
    );
    return 0;
}

bool
send_ping
(
    lo_address engine,
    const std::string & callback_url,
    probe_state & state
)
{
    const unsigned serial { state.ping_serial() };
    if
    (
        lo_send
        (
            engine, "/ping", "ss",
            callback_url.c_str(), "/seq66-test/ping"
        ) < 0
    )
    {
        return false;
    }
    return state.wait_ping_after(serial);
}

bool
wait_for_loop_count
(
    lo_address engine,
    const std::string & callback_url,
    probe_state & state,
    int expected,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)
)
{
    const auto deadline { std::chrono::steady_clock::now() + timeout };
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (send_ping(engine, callback_url, state))
        {
            if (state.loop_count() == expected)
                return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

bool
get_loop_value
(
    lo_address engine,
    const std::string & callback_url,
    probe_state & state,
    int loop_index,
    const std::string & control
)
{
    const unsigned serial { state.value_serial() };
    const std::string path
    {
        "/sl/" + std::to_string(loop_index) + "/get"
    };
    if
    (
        lo_send
        (
            engine, path.c_str(), "sss", control.c_str(),
            callback_url.c_str(), "/seq66-test/value"
        ) < 0
    )
    {
        return false;
    }
    return state.wait_value_after(serial);
}

bool
near (float left, float right, float epsilon = 1.0e-4f)
{
    return std::fabs(left - right) <= epsilon;
}

bool
wait_for_loop_value
(
    lo_address engine,
    const std::string & callback_url,
    probe_state & state,
    int loop_index,
    const std::string & control,
    float expected,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(3000)
)
{
    const auto deadline { std::chrono::steady_clock::now() + timeout };
    float last_value { 0.0f };
    bool observed { false };

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (get_loop_value(engine, callback_url, state, loop_index, control))
        {
            if
            (
                state.loop_index() == loop_index &&
                state.control() == control
            )
            {
                last_value = state.value();
                observed = true;
                if (near(last_value, expected))
                    return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::cerr
        << "Timed out observing loop " << loop_index
        << " control '" << control << "' at " << expected;
    if (observed)
        std::cerr << "; last observed value was " << last_value;
    else
        std::cerr << "; no matching callback was observed";

    std::cerr << std::endl;
    return false;
}

}           // namespace anonymous

int
main ()
{
    const char * endpoint_env { std::getenv("SEQ66_SL_TEST_URL") };
    const std::string endpoint
    {
        endpoint_env ? endpoint_env : "osc.udp://127.0.0.1:19953/"
    };

    lo_server_thread callback_server
    {
        lo_server_thread_new("19952", osc_error)
    };
    assert(callback_server != nullptr);

    probe_state state;
    lo_server_thread_add_method
    (
        callback_server, "/seq66-test/ping", "ssi",
        ping_handler, &state
    );
    lo_server_thread_add_method
    (
        callback_server, "/seq66-test/value", "isf",
        value_handler, &state
    );
    assert(lo_server_thread_start(callback_server) == 0);

    char * callback_url_raw
    {
        lo_server_thread_get_url(callback_server)
    };
    assert(callback_url_raw != nullptr);
    const std::string callback_url { callback_url_raw };
    std::free(callback_url_raw);

    lo_address engine { lo_address_new_from_url(endpoint.c_str()) };
    assert(engine != nullptr);

    assert(send_ping(engine, callback_url, state));
    assert(! state.version().empty());
    assert(state.loop_count() == 0);

    assert(lo_send(engine, "/loop_add", "if", 1, 5.0f) >= 0);
    assert(wait_for_loop_count(engine, callback_url, state, 1));

    assert
    (
        wait_for_loop_value
        (
            engine, callback_url, state, 0, "channel_count", 1.0f
        )
    );

    /*
     * SooperLooper applies loop controls through its engine/audio cycle. A
     * successful OSC send is therefore not synchronous confirmation. Poll the
     * observed value until the engine reports it or the bounded deadline ends.
     */

    assert(lo_send(engine, "/sl/0/set", "sf", "wet", 0.75f) >= 0);
    assert
    (
        wait_for_loop_value
        (
            engine, callback_url, state, 0, "wet", 0.75f
        )
    );

    assert(lo_send(engine, "/loop_del", "i", -1) >= 0);
    assert(wait_for_loop_count(engine, callback_url, state, 0));

    assert(lo_send(engine, "/quit", "") >= 0);

    lo_address_free(engine);
    lo_server_thread_stop(callback_server);
    lo_server_thread_free(callback_server);
    return 0;
}
