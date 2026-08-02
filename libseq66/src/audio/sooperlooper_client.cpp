/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_client.cpp
 *
 *  Non-realtime OSC control for a headless SooperLooper engine.
 */

#include "seq66-config.h"

#include <cmath>
#include <string>

#if SEQ66_SOOPERLOOPER_SUPPORT
#include <lo/lo.h>
#endif

#include "audio/sooperlooper_client.hpp"

namespace seq66
{

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
    }
    return "";
}

class sooperlooper_client::implementation
{

private:

    std::string m_endpoint;
    std::string m_last_error;

#if SEQ66_SOOPERLOOPER_SUPPORT
    lo_address m_address;
#endif

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
#else
        (void) value;
        m_last_error = "Seq66 was built without SooperLooper/liblo support";
        return false;
#endif
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
            const char * text { lo_address_errstr(m_address) };
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
        const std::string & path, const std::string & name, float value
    )
    {
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (! ready())
            return report_send(-1);

        return report_send
        (
            lo_send(m_address, path.c_str(), "sf", name.c_str(), value)
        );
#else
        (void) path;
        (void) name;
        (void) value;
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
};

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
    const char * name { command_name(command) };
    if (! valid_index || ! name[0])
        return false;

    std::string path { "/sl/" + std::to_string(loop_index) + "/hit" };
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

    std::string path { "/sl/" + std::to_string(loop_index) + "/set" };
    return m_impl->send_string_float(path, control, value);
}

bool
sooperlooper_client::set_global_control
(
    const std::string & control, float value
)
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
    int loop { clip.runtime_loop_index() };
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
            set_loop_control
            (
                loop, "stretch_ratio",
                float(clip.time_stretch_ratio(target_bpm))
            );
        break;
    }
    return result;
}

}           // namespace seq66

/*
 * sooperlooper_client.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
