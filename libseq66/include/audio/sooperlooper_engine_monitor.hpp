/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_engine_monitor.hpp
 *
 *  Engine lifecycle monitor for SooperLooper discovery and health tracking.
 *
 *  This module tracks engine reachability (ping), readiness (version,
 *  topology, subscriptions), and health (stale detection via missed
 *  pings).  It does NOT own the client or the receiver; it is fed
 *  externally with ping results and subscription confirmations.
 */

#ifndef SEQ66_SOOPERLOOPER_ENGINE_MONITOR_HPP
#define SEQ66_SOOPERLOOPER_ENGINE_MONITOR_HPP

#include <cstdint>
#include <string>
#include <chrono>

namespace seq66
{

/**
 *  Engine lifecycle state.
 *
 *  State machine:
 *      disabled -> starting -> reconciling -> ready
 *                                          |     |
 *                                          v     v
 *                                        stale --+-> engine_offline
 *                                          |
 *                                          v
 *                                        restarting
 */
enum class engine_state
{
    disabled,           /**< Engine not started or explicitly stopped.    */
    starting,           /**< Ping sent, waiting for reply.               */
    reconciling,        /**< Ping replied; verifying version/topology.   */
    ready,              /**< Fully operational, receiving feedback.      */
    stale,              /**< Missed health checks beyond threshold.      */
    engine_offline,     /**< Confirmed unreachable or process exited.    */
    restarting          /**< Attempting automatic restart.               */
};

/**
 *  Engine health monitor.
 *
 *  Tracks the lifecycle of a SooperLooper engine connection.
 *  Ping scheduling, deadline management, stale detection, and
 *  readiness transitions are handled here.  Protocol operations
 *  (ping, subscribe) remain in the client; this class owns the
 *  scheduling and state machine.
 *
 *  Thread safety: all methods are safe to call from a single
 *  coordinator thread.  No concurrent access assumed.
 */
class sooperlooper_engine_monitor
{
public:
    /**
     *  Configuration for health monitoring deadlines.
     */
    struct config
    {
        int ping_interval_ms{1000};         /**< How often to ping.        */
        int ping_timeout_ms{1000};          /**< Max wait for ping reply.  */
        int startup_deadline_ms{5000};      /**< Max startup time.         */
        int stale_threshold{3};             /**< Missed pings before stale.*/
        int restart_cooldown_ms{2000};      /**< Min time between restarts.*/
    };

private:
    /** Current engine state. */
    engine_state m_state{engine_state::disabled};

    /** Configuration deadlines. */
    config m_config;

    /** Engine version string from last successful ping. */
    std::string m_version;

    /** Loop count from last successful ping. */
    int m_loop_count{0};

    /** Timestamp of last successful ping reply (steady_clock). */
    std::chrono::steady_clock::time_point m_last_ping_reply{};

    /** Timestamp of last ping request sent. */
    std::chrono::steady_clock::time_point m_last_ping_sent{};

    /** Number of consecutive missed pings. */
    int m_missed_pings{0};

    /** Number of successful pings since last restart. */
    int m_successful_pings{0};

public:
    sooperlooper_engine_monitor () = default;
    explicit sooperlooper_engine_monitor (const config & cfg);
    ~sooperlooper_engine_monitor () = default;

    /**
     *  Update configuration.
     */
    void set_config (const config & cfg);

    /**
     *  Get current configuration.
     */
    const config & get_config () const;

    /**
     *  Record that a ping was sent.
     */
    void ping_sent ();

    /**
     *  Record a successful ping reply.
     *
     *  \param version      Engine version string.
     *  \param loop_count   Number of loops.
     */
    void ping_reply (const std::string & version, int loop_count);

    /**
     *  Record a missed ping (timeout with no reply).
     */
    void ping_missed ();

    /**
     *  Evaluate health and advance the state machine.
     *
     *  Call this periodically (e.g. every 100ms) to check deadlines
     *  and transition states.
     *
     *  \return The current state after evaluation.
     */
    engine_state evaluate ();

    /**
     *  Get the current engine state.
     */
    engine_state state () const;

    /**
     *  Force a state transition (for testing or explicit lifecycle control).
     */
    void force_state (engine_state s);

    /**
     *  Get the engine version from the last successful ping.
     */
    const std::string & version () const;

    /**
     *  Get the loop count from the last successful ping.
     */
    int loop_count () const;

    /**
     *  Return true if the engine is in a usable state (ready).
     */
    bool is_ready () const;

    /**
     *  Return true if the engine is unreachable (stale or offline).
     */
    bool is_unreachable () const;

    /**
     *  Reset the monitor to disabled state.
     */
    void reset ();
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_ENGINE_MONITOR_HPP

/*
 * sooperlooper_engine_monitor.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
