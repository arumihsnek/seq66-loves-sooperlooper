#if ! defined SEQ66_SOOPERLOOPER_CLIENT_HPP
#define SEQ66_SOOPERLOOPER_CLIENT_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_client.hpp
 *
 *  Small non-realtime OSC adapter for a headless SooperLooper engine.
 */

#include <memory>
#include <string>

#include "audio/audio_clip.hpp"

namespace seq66
{

enum class sooperlooper_command
{
    record,
    overdub,
    multiply,
    insert,
    replace,
    reverse,
    mute,
    undo,
    redo,
    one_shot,
    trigger,
    substitute,
    pause,
    solo,
    mute_on,
    mute_off
};

class sooperlooper_client
{

private:

    class implementation;
    std::unique_ptr<implementation> m_impl;

public:

    explicit sooperlooper_client
    (
        const std::string & endpoint = "osc.udp://127.0.0.1:9951/"
    );

    ~sooperlooper_client ();

    sooperlooper_client (const sooperlooper_client &) = delete;
    sooperlooper_client & operator = (const sooperlooper_client &) = delete;

    static bool compiled_support ();

    bool endpoint (const std::string & value);
    const std::string & endpoint () const;
    const std::string & last_error () const;
    bool ready () const;

    bool hit (int loop_index, sooperlooper_command command);
    bool set_loop_control
    (
        int loop_index, const std::string & control, float value
    );
    bool set_global_control (const std::string & control, float value);

    bool add_loop (int channels, float minimum_seconds = 40.0f);
    bool delete_last_loop ();

    /**
     *  Applies Seq66's three tempo policies to one existing loop.
     *  It does not start playback or recording.
     */

    bool apply_sync_policy (const audio_clip & clip, double target_bpm);
};

}           // namespace seq66

#endif      // SEQ66_SOOPERLOOPER_CLIENT_HPP

/*
 * sooperlooper_client.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
