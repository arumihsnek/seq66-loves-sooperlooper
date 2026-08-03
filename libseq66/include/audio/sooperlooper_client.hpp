#if ! defined SEQ66_SOOPERLOOPER_CLIENT_HPP
#define SEQ66_SOOPERLOOPER_CLIENT_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_client.hpp
 *
 *  Small non-realtime OSC adapter for a headless SooperLooper engine.
 *
 *  M1-005B: Subscription wire protocol now matches the SooperLooper contract:
 *    /sl/<index>/register_update      s:control  s:return_url  s:return_path
 *    /sl/<index>/register_auto_update s:control  i:interval_ms s:return_url  s:return_path
 *    /sl/<index>/unregister_update    s:control  s:return_url  s:return_path
 *    /sl/<index>/unregister_auto_update s:control s:return_url s:return_path
 *    /register_update      s:control  s:return_url  s:return_path
 *    /register_auto_update s:control  i:interval_ms s:return_url  s:return_path
 *    /unregister_update    s:control  s:return_url  s:return_path
 *    /unregister_auto_update s:control s:return_url s:return_path
 *
 *  Callbacks are delivered through a shared sooperlooper_receiver that the
 *  caller provides.  Subscriptions are associated with an engine generation
 *  and cancelled during shutdown/restart.
 */

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "audio/audio_clip.hpp"
#include "audio/sooperlooper_protocol.hpp"

namespace seq66
{

class sooperlooper_receiver;

/**
 *  Public API: retains string-based methods for backward compatibility,
 *  adds new typed overloads for loop and global controls.
 *
 *  M1-005B: Subscription methods now send the full SooperLooper wire
 *  protocol including return_url and return_path.  A shared receiver
 *  provides callback delivery.  Subscriptions are generation-scoped
 *  and cancelled on shutdown/restart.
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

    /* -----------------------------------------------------------------
     *  Receiver integration (M1-005B)
     * ----------------------------------------------------------------- */

    /**
     *  Bind a shared receiver for subscription callbacks.
     *
     *  The receiver must be started before subscriptions are created.
     *  The client does not own the receiver; the caller manages its
     *  lifetime.
     *
     *  The receiver's port is used to construct the return_url for
     *  subscription messages.
     */
    void set_receiver (sooperlooper_receiver * receiver);

    /** Return the currently bound receiver (may be nullptr). */
    sooperlooper_receiver * receiver () const;

    /* -----------------------------------------------------------------
     *  Engine generation (M1-005B)
     * ----------------------------------------------------------------- */

    /**
     *  Set the current engine generation.  Subscriptions are associated
     *  with the generation at creation time.  On generation change
     *  (restart), all subscriptions from the previous generation are
     *  considered stale and should be cancelled via cancel_subscriptions().
     */
    void set_generation (std::uint64_t generation);

    /** Get the current engine generation. */
    std::uint64_t generation () const;

    /* -----------------------------------------------------------------
     *  Subscription API (M1-005B — corrected wire protocol)
     * ----------------------------------------------------------------- */

    /**
     *  Subscribe to a per-loop control change notification.
     *
     *  Sends: /sl/<index>/register_update  s:control  s:return_url  s:return_path
     *
     *  \param loop_index   The loop index.
     *  \param control      The control to subscribe to.
     *  \return true if the subscribe message was sent.
     */
    bool subscribe_loop (int loop_index, loop_control control);

    /**
     *  Subscribe to a per-loop auto-update at the given interval.
     *
     *  Sends: /sl/<index>/register_auto_update  s:control  i:interval_ms  s:return_url  s:return_path
     *
     *  \param loop_index   The loop index.
     *  \param control      The control to subscribe to.
     *  \param interval_ms  Update interval (10..100 ms, sent as integer).
     *  \return true if the subscribe message was sent.
     */
    bool subscribe_loop_auto (int loop_index, loop_control control,
                              int interval_ms = 50);

    /**
     *  Subscribe to a global control change notification.
     *
     *  Sends: /register_update  s:control  s:return_url  s:return_path
     *
     *  \param control      The global control to subscribe to.
     *  \return true if the subscribe message was sent.
     */
    bool subscribe_global (global_control control);

    /**
     *  Subscribe to a global auto-update at the given interval.
     *
     *  Sends: /register_auto_update  s:control  i:interval_ms  s:return_url  s:return_path
     *
     *  \param control      The global control to subscribe to.
     *  \param interval_ms  Update interval (10..100 ms, sent as integer).
     *  \return true if the subscribe message was sent.
     */
    bool subscribe_global_auto (global_control control,
                               int interval_ms = 50);

    /* -----------------------------------------------------------------
     *  Unsubscribe API (M1-005B)
     * ----------------------------------------------------------------- */

    /**
     *  Unsubscribe from a per-loop control change notification.
     *
     *  Sends: /sl/<index>/unregister_update  s:control  s:return_url  s:return_path
     */
    bool unsubscribe_loop (int loop_index, loop_control control);

    /**
     *  Unsubscribe from a per-loop auto-update.
     *
     *  Sends: /sl/<index>/unregister_auto_update  s:control  s:return_url  s:return_path
     */
    bool unsubscribe_loop_auto (int loop_index, loop_control control);

    /**
     *  Unsubscribe from a global control change notification.
     *
     *  Sends: /unregister_update  s:control  s:return_url  s:return_path
     */
    bool unsubscribe_global (global_control control);

    /**
     *  Unsubscribe from a global auto-update.
     *
     *  Sends: /unregister_auto_update  s:control  s:return_url  s:return_path
     */
    bool unsubscribe_global_auto (global_control control);

    /* -----------------------------------------------------------------
     *  Subscription lifecycle (M1-005B)
     * ----------------------------------------------------------------- */

    /**
     *  Cancel all active subscriptions for the current generation.
     *
     *  Sends unregister messages for every tracked subscription.
     *  Called during shutdown or restart.
     */
    void cancel_subscriptions ();

    /**
     *  Return the number of active subscriptions for the current
     *  generation.
     */
    std::size_t active_subscription_count () const;

    /**
     *  Return callback path for subscription callbacks.
     *  Returns "/reply" by default; configurable for testing.
     */
    const std::string & callback_path () const;

    /**
     *  Set the callback path used in subscription messages.
     */
    void set_callback_path (const std::string & path);

    /**
     *  Return the callback URL (return_url) that will be sent in
     *  subscription messages.  Returns empty if no receiver is set.
     */
    std::string callback_url () const;
};  // class sooperlooper_client

}           // namespace seq66

#endif      // SEQ66_SOOPERLOOPER_CLIENT_HPP

/*
 * sooperlooper_client.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
*/
