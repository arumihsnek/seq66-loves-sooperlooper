/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_client.cpp
 *
 *  Non-realtime OSC control for a headless SooperLooper engine.
 */

#include "seq66-config.h"

#include <chrono>
#include <cmath>
#include <string>
#include <unordered_set>
#include <vector>

#if SEQ66_SOOPERLOOPER_SUPPORT
#include <lo/lo.h>
#endif

#include "audio/sooperlooper_client.hpp"
#include "audio/sooperlooper_protocol.hpp"
#include "audio/sooperlooper_receiver.hpp"

namespace seq66
{

/* -------------------------------------------------------------------------
 *  Helper: command_name (unchanged)
 * ------------------------------------------------------------------------- */

static const char *
command_name (sooperlooper_command command)
{
    switch (command)
    {
        case sooperlooper_command::record:       return "record";
        case sooperlooper_command::overdub:      return "overdub";
        case sooperlooper_command::multiply:     return "multiply";
        case sooperlooper_command::insert:       return "insert";
        case sooperlooper_command::replace:      return "replace";
        case sooperlooper_command::reverse:      return "reverse";
        case sooperlooper_command::mute:         return "mute";
        case sooperlooper_command::undo:         return "undo";
        case sooperlooper_command::redo:         return "redo";
        case sooperlooper_command::one_shot:     return "oneshot";
        case sooperlooper_command::trigger:      return "trigger";
        case sooperlooper_command::substitute:   return "substitute";
        case sooperlooper_command::pause:        return "pause";
        case sooperlooper_command::solo:         return "solo";
        case sooperlooper_command::mute_on:      return "mute_on";
        case sooperlooper_command::mute_off:     return "mute_off";
        default:                                 break;
    }
    return "";
}

/* -------------------------------------------------------------------------
 *  Subscription record for tracking active subscriptions
 * ------------------------------------------------------------------------- */

struct subscription_record
{
    std::string osc_path;
    std::string control_name;
    bool is_auto;
    int interval_ms;
    std::uint64_t generation;
};

/* -------------------------------------------------------------------------
 *  pimpl implementation
 * ------------------------------------------------------------------------- */

class sooperlooper_client::implementation
{
private:

    std::string m_endpoint;
    std::string m_last_error;
    std::string m_callback_path{"/reply"};

#if SEQ66_SOOPERLOOPER_SUPPORT
    lo_address m_address;
#endif

    /** Shared receiver for subscription callbacks (not owned). */
    sooperlooper_receiver * m_receiver{nullptr};

    /** Current engine generation. */
    std::uint64_t m_generation{0};

    /** Active subscriptions for cancellation tracking. */
    std::vector<subscription_record> m_subscriptions;

public:

    implementation () :
        m_endpoint     (),
        m_last_error   ()
#if SEQ66_SOOPERLOOPER_SUPPORT
        , m_address    (nullptr)
#endif
    {
        // No code needed.
    }

    ~implementation ()
    {
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (m_address)
            lo_address_free(m_address);
#endif
    }

    bool endpoint (const std::string & value)
    {
        m_endpoint = value;
        m_last_error.clear();

#if SEQ66_SOOPERLOOPER_SUPPORT
        if (m_address)
        {
            lo_address_free(m_address);
            m_address = nullptr;
        }
#endif

        if (value.empty())
        {
            m_last_error = "Empty SooperLooper OSC endpoint";
            return false;
        }

        m_address = lo_address_new_from_url(value.c_str());
        if (! m_address)
        {
            m_last_error = "Invalid SooperLooper OSC endpoint";
            return false;
        }
        return true;
    }

    const std::string & endpoint () const
    {
        return m_endpoint;
    }

    const std::string & last_error () const
    {
        return m_last_error;
    }

    bool ready () const
    {
#if SEQ66_SOOPERLOOPER_SUPPORT
        return m_address != nullptr;
#else
        return false;
#endif
    }

    void set_error (const std::string & msg)
    {
        m_last_error = msg;
    }

    void clear_error ()
    {
        m_last_error.clear();
    }

#if SEQ66_SOOPERLOOPER_SUPPORT
    lo_address get_address () const { return m_address; }
#endif

    bool report_send (int result)
    {
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (result >= 0)
        {
            m_last_error.clear();
            return true;
        }

        if (m_address)
        {
            const char * text { lo_address_errstr(m_address) } ;
            m_last_error = text ? text : "Unknown OSC send failure";
        }
        else
            m_last_error = "SooperLooper OSC endpoint is not configured";
#else
        (void) result;
        m_last_error = "Seq66 was built without SooperLooper/liblo support";
#endif
        return false;
    }

    bool send_string (const std::string & path, const std::string & value)
    {
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (! ready())
            return report_send(-1);

        return report_send(lo_send(m_address, path.c_str(), "s", value.c_str()));
#else
        (void) path;
        (void) value;
        return report_send(-1);
#endif
    }

    bool send_string_float
    (
        const std::string & path,
        const std::string & name,
        float value
    )
    {
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (! ready())
            return report_send(-1);

        return report_send(lo_send(m_address, path.c_str(), "sf", name.c_str(), value));
#else
        (void) path;
        (void) name;
        (void) value;
        return report_send(-1);
#endif
    }

    /**
     *  M1-005B: Send a string and an integer.
     *  Used for register_auto_update: s:control  i:interval_ms  s:return_url  s:return_path
     */
    bool send_string_int
    (
        const std::string & path,
        const std::string & control,
        int value
    )
    {
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (! ready())
            return report_send(-1);

        return report_send(lo_send(m_address, path.c_str(), "si", control.c_str(), value));
#else
        (void) path;
        (void) control;
        (void) value;
        return report_send(-1);
#endif
    }

    /**
     *  M1-005B: Send a string, an integer, and two strings.
     *  Used for register_auto_update: s:control  i:interval_ms  s:return_url  s:return_path
     */
    bool send_string_int_string_string
    (
        const std::string & path,
        const std::string & control,
        int interval_ms,
        const std::string & return_url,
        const std::string & return_path
    )
    {
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (! ready())
            return report_send(-1);

        return report_send(lo_send(m_address, path.c_str(), "siss",
            control.c_str(), interval_ms,
            return_url.c_str(), return_path.c_str()));
#else
        (void) path;
        (void) control;
        (void) interval_ms;
        (void) return_url;
        (void) return_path;
        return report_send(-1);
#endif
    }

    /**
     *  M1-005B: Send three strings.
     *  Used for register_update/unregister: s:control  s:return_url  s:return_path
     */
    bool send_string_string_string
    (
        const std::string & path,
        const std::string & control,
        const std::string & return_url,
        const std::string & return_path
    )
    {
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (! ready())
            return report_send(-1);

        return report_send(lo_send(m_address, path.c_str(), "sss",
            control.c_str(), return_url.c_str(), return_path.c_str()));
#else
        (void) path;
        (void) control;
        (void) return_url;
        (void) return_path;
        return report_send(-1);
#endif
    }

    bool send_int_float (const std::string & path, int first, float second)
    {
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (! ready())
            return report_send(-1);

        return report_send(lo_send(m_address, path.c_str(), "if", first, second));
#else
        (void) path;
        (void) first;
        (void) second;
        return report_send(-1);
#endif
    }

    bool send_int (const std::string & path, int value)
    {
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (! ready())
            return report_send(-1);

        return report_send(lo_send(m_address, path.c_str(), "i", value));
#else
        (void) path;
        (void) value;
        return report_send(-1);
#endif
    }

    /* -----------------------------------------------------------------
     *  Receiver and generation (M1-005B)
     * ----------------------------------------------------------------- */

    void set_receiver (sooperlooper_receiver * r)
    {
        m_receiver = r;
    }

    sooperlooper_receiver * get_receiver () const
    {
        return m_receiver;
    }

    void set_generation (std::uint64_t gen)
    {
        m_generation = gen;
    }

    std::uint64_t get_generation () const
    {
        return m_generation;
    }

    const std::string & callback_path () const
    {
        return m_callback_path;
    }

    void set_callback_path (const std::string & path)
    {
        m_callback_path = path;
    }

    /**
     *  Build the return_url from the receiver's port.
     *  Returns empty string if no receiver is bound.
     */
    std::string build_return_url () const
    {
        if (! m_receiver || ! m_receiver->started())
            return "";
        return "osc.udp://127.0.0.1:" + std::to_string(m_receiver->port());
    }

    /* -----------------------------------------------------------------
     *  Subscription tracking (M1-005B)
     * ----------------------------------------------------------------- */

    void track_subscription
    (
        const std::string & osc_path,
        const std::string & control_name,
        bool is_auto,
        int interval_ms
    )
    {
        subscription_record rec;
        rec.osc_path = osc_path;
        rec.control_name = control_name;
        rec.is_auto = is_auto;
        rec.interval_ms = interval_ms;
        rec.generation = m_generation;
        m_subscriptions.push_back(std::move(rec));
    }

    std::size_t subscription_count () const
    {
        return m_subscriptions.size();
    }

    /**
     *  Build the unregister path from a register path.
     *  Replaces "register" with "unregister" in the last path segment.
     *
     *  Examples:
     *    /sl/0/register_update          -> /sl/0/unregister_update
     *    /sl/0/register_auto_update     -> /sl/0/unregister_auto_update
     *    /register_update               -> /unregister_update
     *    /register_auto_update          -> /unregister_auto_update
     */
    static std::string make_unregister_path (const std::string & register_path)
    {
        /* Find "register" in the path and replace with "unregister".
         * We search for the last occurrence since the path structure is
         * /sl/<index>/register_* or /register_*. */
        std::string result = register_path;
        std::size_t pos = result.rfind("register");
        if (pos != std::string::npos)
        {
            result.replace(pos, 8, "unregister");  // 8 = strlen("register")
        }
        return result;
    }

    /**
     *  Cancel all subscriptions for the current generation.
     *  Sends unregister messages for each tracked subscription.
     */
    void cancel_all_subscriptions ()
    {
        std::string return_url = build_return_url();
        const std::string & return_path = m_callback_path;

        for (const auto & rec : m_subscriptions)
        {
            if (rec.generation != m_generation)
                continue;
            if (return_url.empty())
                continue;

            std::string unregister_path = make_unregister_path(rec.osc_path);
            (void) send_string_string_string(
                unregister_path, rec.control_name, return_url, return_path);
        }
        m_subscriptions.clear();
    }

    /**
     *  Clear subscriptions without sending unregister messages.
     *  Used during generation change (restart) when the old engine
     *  is no longer reachable.
     */
    void clear_stale_subscriptions ()
    {
        m_subscriptions.clear();
    }
};

/* -------------------------------------------------------------------------
 *  sooperlooper_client public methods
 * ------------------------------------------------------------------------- */

sooperlooper_client::sooperlooper_client (const std::string & endpoint) :
    m_impl     (new implementation())
{
    (void) m_impl->endpoint(endpoint);
}

sooperlooper_client::~sooperlooper_client () = default;

bool
sooperlooper_client::compiled_support ()
{
#if SEQ66_SOOPERLOOPER_SUPPORT
    return true;
#else
    return false;
#endif
}

bool
sooperlooper_client::endpoint (const std::string & value)
{
    return m_impl->endpoint(value);
}

const std::string &
sooperlooper_client::endpoint () const
{
    return m_impl->endpoint();
}

const std::string &
sooperlooper_client::last_error () const
{
    return m_impl->last_error();
}

bool
sooperlooper_client::ready () const
{
    return m_impl->ready();
}

/* -----------------------------------------------------------------
 *  String-based original API (preserved for compatibility)
 * ----------------------------------------------------------------- */

bool
sooperlooper_client::hit
(
    int loop_index, sooperlooper_command command
)
{
    bool valid_index
    {
        loop_index >= 0 || loop_index == -1 || loop_index == -3
    };
    const char * name { command_name(command) } ;
    if (! valid_index || ! name[0])
        return false;

    std::string path { "/sl/" + std::to_string(loop_index) + "/hit" } ;
    return m_impl->send_string(path, name);
}

bool
sooperlooper_client::set_loop_control
(
    int loop_index, const std::string & control, float value
)
{
    bool valid_index
    {
        loop_index >= 0 || loop_index == -1 || loop_index == -3
    };
    if (! valid_index || control.empty() || ! std::isfinite(value))
        return false;

    std::string path { "/sl/" + std::to_string(loop_index) + "/set" } ;
    return m_impl->send_string_float(path, control, value);
}

bool
sooperlooper_client::set_global_control (const std::string & control, float value)
{
    return ! control.empty() && std::isfinite(value) ?
        m_impl->send_string_float("/set", control, value) :
        false ;
}

bool
sooperlooper_client::add_loop (int channels, float minimum_seconds)
{
    return
    (
        channels >= 1 && channels <= 16 && std::isfinite(minimum_seconds) &&
        minimum_seconds > 0.0f
    ) ?
        m_impl->send_int_float("/loop_add", channels, minimum_seconds) :
        false ;
}

bool
sooperlooper_client::delete_last_loop ()
{
    return m_impl->send_int("/loop_del", -1);
}

bool
sooperlooper_client::apply_sync_policy
(
    const audio_clip & clip, double target_bpm
)
{
    int loop { clip.runtime_loop_index() } ;
    if (loop < 0 || ! clip.supports_tempo(target_bpm))
        return false;

    bool result
    {
        set_global_control("tempo", float(target_bpm)) &&
        set_loop_control(loop, "pitch_shift", float(clip.pitch_shift())) &&
        set_loop_control(loop, "quantize", 1.0f) &&
        set_loop_control(loop, "round", 1.0f)
    };

    switch (clip.sync_mode())
    {
        case audio_sync_mode::free:
            result = result &&
                set_loop_control(loop, "sync", 0.0f) &&
                set_loop_control(loop, "playback_sync", 0.0f) &&
                set_loop_control(loop, "tempo_stretch", 0.0f) &&
                set_loop_control(loop, "use_rate", 0.0f) &&
                set_loop_control(loop, "rate", 1.0f) &&
                set_loop_control(loop, "stretch_ratio", 1.0f);
            break;

        case audio_sync_mode::tape:
            result = result &&
                set_loop_control(loop, "sync", 1.0f) &&
                set_loop_control(loop, "playback_sync", 1.0f) &&
                set_loop_control(loop, "tempo_stretch", 0.0f) &&
                set_loop_control(loop, "stretch_ratio", 1.0f) &&
                set_loop_control(loop, "use_rate", 1.0f) &&
                set_loop_control(loop, "rate", float(clip.playback_rate(target_bpm)));
            break;

        case audio_sync_mode::elastic:
            result = result &&
                set_loop_control(loop, "sync", 1.0f) &&
                set_loop_control(loop, "playback_sync", 1.0f) &&
                set_loop_control(loop, "use_rate", 0.0f) &&
                set_loop_control(loop, "rate", 1.0f) &&
                set_loop_control(loop, "tempo_stretch", 1.0f) &&
                set_loop_control(loop, "stretch_ratio",
                                 float(clip.time_stretch_ratio(target_bpm)));
            break;
    }
    return result;
}

/* -----------------------------------------------------------------
 *  New typed overloads (delegate to protocol helpers)
 * ----------------------------------------------------------------- */

bool
sooperlooper_client::set_loop_control
(
    int loop_index, loop_control control, float value
)
{
    if (! m_impl->ready())
        return false;
    if (! is_in_range(control, value))
        return false;
    return m_impl->send_string_float(
                "/sl/" + std::to_string(loop_index) + "/set",
                to_string(control), value);
}

bool
sooperlooper_client::set_global_control (global_control control, float value)
{
    if (! m_impl->ready())
        return false;
    if (! is_in_range(control, value))
        return false;
    return m_impl->send_string_float("/set", to_string(control), value);
}

/* -----------------------------------------------------------------
 *  Engine discovery and health (real OSC transport)
 * ----------------------------------------------------------------- */

#if SEQ66_SOOPERLOOPER_SUPPORT

struct ping_context
{
    lo_server server{nullptr};
    bool replied{false};
    std::string engine_url;
    std::string version;
    int loop_count{0};
};

static int
ping_reply_handler (const char * /* path */, const char * types,
                    lo_arg ** argv, int argc,
                    void * /* data */, void * user_data)
{
    auto * ctx = static_cast<ping_context *>(user_data) ;
    if (! ctx || argc < 3)
        return 0;
    if (types[0] != 's' || types[1] != 's' || types[2] != 'i')
        return 0;
    ctx->engine_url = &argv[0]->s;
    ctx->version = &argv[1]->s;
    ctx->loop_count = argv[2]->i;
    ctx->replied = true;
    return 0;
}

#endif

bool
sooperlooper_client::ping (std::string & version, int & loop_count,
                           int timeout_ms)
{
#if SEQ66_SOOPERLOOPER_SUPPORT
    if (! m_impl->ready())
        return false;

    ping_context ctx;
    ctx.server = lo_server_new(nullptr, nullptr);
    if (! ctx.server)
    {
        m_impl->set_error("Failed to create temporary OSC server");
        return false;
    }

    lo_server_add_method(ctx.server, "/reply", "ssi",
                         ping_reply_handler, &ctx);

    int port = lo_server_get_port(ctx.server);
    std::string return_url =
        "osc.udp://127.0.0.1:" + std::to_string(port);

    int result = lo_send(m_impl->get_address(), "/ping", "ss",
                         return_url.c_str(), "/reply");
    if (result < 0)
    {
        m_impl->set_error("Failed to send ping");
        lo_server_free(ctx.server);
        return false;
    }

    auto start = std::chrono::steady_clock::now();
    while (! ctx.replied)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeout_ms)
        {
            m_impl->set_error("Ping reply timeout");
            lo_server_free(ctx.server);
            return false;
        }
        lo_server_recv_noblock(ctx.server, 50);
    }

    lo_server_free(ctx.server);
    version = ctx.version;
    loop_count = ctx.loop_count;
    m_impl->clear_error();
    return true;
#else
    (void) version;
    (void) loop_count;
    (void) timeout_ms;
    m_impl->set_error("Built without SooperLooper support");
    return false;
#endif
}

/* -----------------------------------------------------------------
 *  Receiver integration (M1-005B)
 * ----------------------------------------------------------------- */

void
sooperlooper_client::set_receiver (sooperlooper_receiver * receiver)
{
    m_impl->set_receiver(receiver);
}

sooperlooper_receiver *
sooperlooper_client::receiver () const
{
    return m_impl->get_receiver();
}

void
sooperlooper_client::set_generation (std::uint64_t generation)
{
    /* When generation changes, clear stale subscriptions from the
     * previous generation without sending unregister (old engine
     * is unreachable). */
    if (generation != m_impl->get_generation())
        m_impl->clear_stale_subscriptions();
    m_impl->set_generation(generation);
}

std::uint64_t
sooperlooper_client::generation () const
{
    return m_impl->get_generation();
}

const std::string &
sooperlooper_client::callback_path () const
{
    return m_impl->callback_path();
}

void
sooperlooper_client::set_callback_path (const std::string & path)
{
    m_impl->set_callback_path(path);
}

std::string
sooperlooper_client::callback_url () const
{
    return m_impl->build_return_url();
}

/* -----------------------------------------------------------------
 *  Subscription API (M1-005B — corrected wire protocol)
 *
 *  SooperLooper contract:
 *    /sl/<index>/register_update      s:control  s:return_url  s:return_path
 *    /sl/<index>/register_auto_update s:control  i:interval_ms s:return_url  s:return_path
 *    /sl/<index>/unregister_update    s:control  s:return_url  s:return_path
 *    /sl/<index>/unregister_auto_update s:control s:return_url s:return_path
 *    /register_update      s:control  s:return_url  s:return_path
 *    /register_auto_update s:control  i:interval_ms s:return_url  s:return_path
 *    /unregister_update    s:control  s:return_url  s:return_path
 *    /unregister_auto_update s:control s:return_url s:return_path
 * ----------------------------------------------------------------- */

bool
sooperlooper_client::subscribe_loop (int loop_index, loop_control control)
{
#if SEQ66_SOOPERLOOPER_SUPPORT
    if (! m_impl->ready())
        return false;

    const char * ctrl = to_string(control);
    if (! ctrl || ctrl[0] == '\0')
    {
        m_impl->set_error("Invalid loop control for subscription");
        return false;
    }

    bool valid_index
    {
        loop_index >= 0 || loop_index == -1 || loop_index == -3
    };
    if (! valid_index)
    {
        m_impl->set_error("Invalid loop index for subscription");
        return false;
    }

    std::string return_url = m_impl->build_return_url();
    if (return_url.empty())
    {
        m_impl->set_error("No receiver bound for subscription callbacks");
        return false;
    }

    std::string path = "/sl/" + std::to_string(loop_index) +
                       "/register_update";
    bool sent = m_impl->send_string_string_string(
        path, ctrl, return_url, m_impl->callback_path());
    if (sent)
    {
        m_impl->track_subscription(path, ctrl, false, 0);
    }
    return sent;
#else
    (void) loop_index;
    (void) control;
    return false;
#endif
}

bool
sooperlooper_client::subscribe_loop_auto (int loop_index,
                                           loop_control control,
                                           int interval_ms)
{
#if SEQ66_SOOPERLOOPER_SUPPORT
    if (! m_impl->ready())
        return false;

    const char * ctrl = to_string(control);
    if (! ctrl || ctrl[0] == '\0')
    {
        m_impl->set_error("Invalid loop control for auto subscription");
        return false;
    }

    if (interval_ms < 10 || interval_ms > 100)
    {
        m_impl->set_error("Auto-update interval must be 10..100 ms");
        return false;
    }

    bool valid_index
    {
        loop_index >= 0 || loop_index == -1 || loop_index == -3
    };
    if (! valid_index)
    {
        m_impl->set_error("Invalid loop index for auto subscription");
        return false;
    }

    std::string return_url = m_impl->build_return_url();
    if (return_url.empty())
    {
        m_impl->set_error("No receiver bound for subscription callbacks");
        return false;
    }

    std::string path = "/sl/" + std::to_string(loop_index) +
                       "/register_auto_update";
    bool sent = m_impl->send_string_int_string_string(
        path, ctrl, interval_ms, return_url, m_impl->callback_path());
    if (sent)
    {
        m_impl->track_subscription(path, ctrl, true, interval_ms);
    }
    return sent;
#else
    (void) loop_index;
    (void) control;
    (void) interval_ms;
    return false;
#endif
}

bool
sooperlooper_client::subscribe_global (global_control control)
{
#if SEQ66_SOOPERLOOPER_SUPPORT
    if (! m_impl->ready())
        return false;

    const char * ctrl = to_string(control);
    if (! ctrl || ctrl[0] == '\0')
    {
        m_impl->set_error("Invalid global control for subscription");
        return false;
    }

    std::string return_url = m_impl->build_return_url();
    if (return_url.empty())
    {
        m_impl->set_error("No receiver bound for subscription callbacks");
        return false;
    }

    bool sent = m_impl->send_string_string_string(
        "/register_update", ctrl, return_url, m_impl->callback_path());
    if (sent)
    {
        m_impl->track_subscription("/register_update", ctrl, false, 0);
    }
    return sent;
#else
    (void) control;
    return false;
#endif
}

bool
sooperlooper_client::subscribe_global_auto (global_control control,
                                            int interval_ms)
{
#if SEQ66_SOOPERLOOPER_SUPPORT
    if (! m_impl->ready())
        return false;

    const char * ctrl = to_string(control);
    if (! ctrl || ctrl[0] == '\0')
    {
        m_impl->set_error("Invalid global control for auto subscription");
        return false;
    }

    if (interval_ms < 10 || interval_ms > 100)
    {
        m_impl->set_error("Auto-update interval must be 10..100 ms");
        return false;
    }

    std::string return_url = m_impl->build_return_url();
    if (return_url.empty())
    {
        m_impl->set_error("No receiver bound for subscription callbacks");
        return false;
    }

    bool sent = m_impl->send_string_int_string_string(
        "/register_auto_update", ctrl, interval_ms,
        return_url, m_impl->callback_path());
    if (sent)
    {
        m_impl->track_subscription(
            "/register_auto_update", ctrl, true, interval_ms);
    }
    return sent;
#else
    (void) control;
    (void) interval_ms;
    return false;
#endif
}

/* -----------------------------------------------------------------
 *  Unsubscribe API (M1-005B)
 * ----------------------------------------------------------------- */

bool
sooperlooper_client::unsubscribe_loop (int loop_index, loop_control control)
{
#if SEQ66_SOOPERLOOPER_SUPPORT
    if (! m_impl->ready())
        return false;

    const char * ctrl = to_string(control);
    if (! ctrl || ctrl[0] == '\0')
    {
        m_impl->set_error("Invalid loop control for unsubscription");
        return false;
    }

    std::string return_url = m_impl->build_return_url();
    if (return_url.empty())
    {
        m_impl->set_error("No receiver bound for unsubscription");
        return false;
    }

    std::string path = "/sl/" + std::to_string(loop_index) +
                       "/unregister_update";
    return m_impl->send_string_string_string(
        path, ctrl, return_url, m_impl->callback_path());
#else
    (void) loop_index;
    (void) control;
    return false;
#endif
}

bool
sooperlooper_client::unsubscribe_loop_auto (int loop_index, loop_control control)
{
#if SEQ66_SOOPERLOOPER_SUPPORT
    if (! m_impl->ready())
        return false;

    const char * ctrl = to_string(control);
    if (! ctrl || ctrl[0] == '\0')
    {
        m_impl->set_error("Invalid loop control for auto unsubscription");
        return false;
    }

    std::string return_url = m_impl->build_return_url();
    if (return_url.empty())
    {
        m_impl->set_error("No receiver bound for auto unsubscription");
        return false;
    }

    std::string path = "/sl/" + std::to_string(loop_index) +
                       "/unregister_auto_update";
    return m_impl->send_string_string_string(
        path, ctrl, return_url, m_impl->callback_path());
#else
    (void) loop_index;
    (void) control;
    return false;
#endif
}

bool
sooperlooper_client::unsubscribe_global (global_control control)
{
#if SEQ66_SOOPERLOOPER_SUPPORT
    if (! m_impl->ready())
        return false;

    const char * ctrl = to_string(control);
    if (! ctrl || ctrl[0] == '\0')
    {
        m_impl->set_error("Invalid global control for unsubscription");
        return false;
    }

    std::string return_url = m_impl->build_return_url();
    if (return_url.empty())
    {
        m_impl->set_error("No receiver bound for unsubscription");
        return false;
    }

    return m_impl->send_string_string_string(
        "/unregister_update", ctrl, return_url, m_impl->callback_path());
#else
    (void) control;
    return false;
#endif
}

bool
sooperlooper_client::unsubscribe_global_auto (global_control control)
{
#if SEQ66_SOOPERLOOPER_SUPPORT
    if (! m_impl->ready())
        return false;

    const char * ctrl = to_string(control);
    if (! ctrl || ctrl[0] == '\0')
    {
        m_impl->set_error("Invalid global control for auto unsubscription");
        return false;
    }

    std::string return_url = m_impl->build_return_url();
    if (return_url.empty())
    {
        m_impl->set_error("No receiver bound for auto unsubscription");
        return false;
    }

    return m_impl->send_string_string_string(
        "/unregister_auto_update", ctrl, return_url, m_impl->callback_path());
#else
    (void) control;
    return false;
#endif
}

/* -----------------------------------------------------------------
 *  Subscription lifecycle (M1-005B)
 * ----------------------------------------------------------------- */

void
sooperlooper_client::cancel_subscriptions ()
{
    m_impl->cancel_all_subscriptions();
}

std::size_t
sooperlooper_client::active_subscription_count () const
{
    return m_impl->subscription_count();
}

}           // namespace seq66

/*
 * sooperlooper_client.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
