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
#include "audio/sooperlooper_protocol.hpp"

namespace seq66
{

/**
 *  Public API: retains string-based methods for backward compatibility,
 *  adds new typed overloads for loop and global controls.
 */
class sooperlooper_client
{
private:

    /* Forward declaration of the pimpl implementation (nested class) */
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

    /* -----------------------------------------------------------------
     *  String-based original API (preserved for compatibility)
     * ----------------------------------------------------------------- */

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

    /* -----------------------------------------------------------------
     *  New typed overloads (preferred for new code)
     * ----------------------------------------------------------------- */

    bool set_loop_control
    (
        int loop_index, loop_control control, float value
    );

    bool set_global_control (global_control control, float value);

    /* -----------------------------------------------------------------
     *  Engine discovery and health
     * ----------------------------------------------------------------- */

    /**
     *  Send a ping request and wait for a reply.
     *
     *  \param[out] version      Engine version string on success.
     *  \param[out] loop_count   Number of loops on success.
     *  \param timeout_ms        Maximum wait for reply.
     *  \return true if a valid reply was received.
     */
    bool ping (std::string & version, int & loop_count,
               int timeout_ms = 1000);

    /**
     *  Subscribe to a per-loop control change notification.
     *
     *  \param loop_index   The loop index.
     *  \param control      The control to subscribe to.
     *  \return true if the subscribe message was sent.
     */
    bool subscribe_loop (int loop_index, loop_control control);

    /**
     *  Subscribe to a per-loop auto-update at the given interval.
     *
     *  \param loop_index   The loop index.
     *  \param control      The control to subscribe to.
     *  \param interval_ms  Update interval (10..100 ms).
     *  \return true if the subscribe message was sent.
     */
    bool subscribe_loop_auto (int loop_index, loop_control control,
                              int interval_ms = 50);

    /**
     *  Subscribe to a global control change notification.
     *
     *  \param control      The global control to subscribe to.
     *  \return true if the subscribe message was sent.
     */
    bool subscribe_global (global_control control);

    /**
     *  Subscribe to a global auto-update at the given interval.
     *
     *  \param control      The global control to subscribe to.
     *  \param interval_ms  Update interval (10..100 ms).
     *  \return true if the subscribe message was sent.
     */
    bool subscribe_global_auto (global_control control,
                               int interval_ms = 50);
};  // class sooperlooper_client

}           // namespace seq66

#endif      // SEQ66_SOOPERLOOPER_CLIENT_HPP

/*
 * sooperlooper_client.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
*/
