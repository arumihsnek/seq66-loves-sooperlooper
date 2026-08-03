#if ! defined SEQ66_SOOPERLOOPER_CRASH_RECONCILER_HPP
#define SEQ66_SOOPERLOOPER_CRASH_RECONCILER_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_crash_reconciler.hpp
 *
 *  Crash detection, backoff, and reconciliation for managed engine lifecycle.
 *
 *  After a child process exits unexpectedly, the reconciler:
 *    - invalidates the engine generation;
 *    - cancels pending operations;
 *    - clears stale observed state;
 *    - applies exponential backoff before restart;
 *    - enters terminal state after exhausting restart budget.
 *
 *  Shutdown-during-startup is handled by transitioning to stopping
 *  and preventing any pending restart.
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "audio/sooperlooper_process_supervisor.hpp"

namespace seq66
{

/**
 *  Pending-operation descriptor.
 *
 *  Each in-flight OSC command or expected callback is registered
 *  as a pending operation.  On crash, all pending operations are
 *  cancelled or marked indeterminate.
 */
struct pending_operation
{
    std::string id;             /**< Unique operation identifier.            */
    std::string description;    /**< Human-readable description.             */
    std::uint64_t generation;   /**< Engine generation at time of submission.*/
    long long submitted_us;     /**< Timestamp of submission.                */
    bool completed{false};      /**< True once the operation completes.      */
};

/**
 *  Reconciler state.
 *
 *  State machine:
 *      idle -> watching -> reconciling -> idle
 *                        |                |
 *                        v                v
 *                     backoff -> watching (after delay)
 *                        |
 *                        v
 *                     terminal
 */
enum class reconciler_state : int
{
    idle,           /**< Not watching any process.                        */
    watching,       /**< Process running, monitoring for crash.           */
    reconciling,    /**< Crash detected, performing cleanup.              */
    backoff,        /**< Waiting before automatic restart attempt.        */
    terminal        /**< Restart budget exhausted; manual intervention.    */
};

/**
 *  Configuration for the crash reconciler.
 */
struct reconciler_config
{
    int base_backoff_ms{1000};      /**< Initial backoff delay.           */
    int max_backoff_ms{30000};      /**< Maximum backoff delay.           */
    double backoff_multiplier{2.0}; /**< Multiplier per restart.          */
    int stable_interval_ms{10000};  /**< Stable running resets backoff.   */
    int max_restarts{3};            /**< Max restarts before terminal.    */
    int startup_grace_ms{5000};     /**< Grace period after launch.       */
};

/**
 *  Crash reconciliation result.
 */
enum class reconcile_result : int
{
    none,           /**< No crash detected or no action taken.           */
    restarted,      /**< Crash reconciled, process restarted.            */
    backoff,        /**< Crash reconciled, waiting before restart.       */
    terminal,       /**< Restart budget exhausted.                       */
    cancelled       /**< Restart cancelled (shutdown during startup).    */
};

/**
 *  Crash detector and reconciler.
 *
 *  Coordinates with the process supervisor, observed cache, and
 *  pending-operation registry to handle crashes gracefully.
 *
 *  Thread safety: all methods safe to call from a single coordinator
 *  thread (same as the supervisor and launcher).
 */
class sooperlooper_crash_reconciler
{
public:
    sooperlooper_crash_reconciler ();
    ~sooperlooper_crash_reconciler ();

    sooperlooper_crash_reconciler (const sooperlooper_crash_reconciler &) = delete;
    sooperlooper_crash_reconciler & operator =
        (const sooperlooper_crash_reconciler &) = delete;

    /**
     *  Configure the reconciler.
     */
    void set_config (const reconciler_config & cfg);

    /** Get current configuration. */
    const reconciler_config & get_config () const;

    /**
     *  Begin watching a launched process.
     *
     *  Call after a successful supervisor.launch().  Records the
     *  generation and marks all pending operations as belonging to
     *  this generation.
     *
     *  \param generation  The engine generation being watched.
     */
    void begin_watching (std::uint64_t generation);

    /**
     *  Stop watching (called on intentional shutdown).
     *
     *  Cancels all pending operations for the current generation
     *  and transitions to idle.
     */
    void stop_watching ();

    /**
     *  Register a pending operation.
     *
     *  \param id             Unique identifier for the operation.
     *  \param description    Human-readable description.
     *  \return true if registered, false if already present.
     */
    bool register_operation
    (
        const std::string & id,
        const std::string & description
    );

    /**
     *  Mark an operation as completed.
     *
     *  \param id  The operation identifier.
     *  \return true if found and marked.
     */
    bool complete_operation (const std::string & id);

    /**
     *  Cancel all pending operations for the current generation.
     *
     *  Called on crash or shutdown.  Operations are not deleted
     *  but marked as cancelled (completed=false).
     */
    void cancel_all_pending ();

    /**
     *  Check for crash and perform reconciliation.
     *
     *  Should be called periodically from the coordinator loop.
     *  If the supervisor reports the child has exited, performs:
     *    1. Cancel all pending operations.
     *    2. Invalidate generation.
     *    3. Compute backoff delay.
     *    4. Either restart or enter terminal state.
     *
     *  \param supervisor  Reference to the process supervisor.
     *  \param on_invalidate_generation  Callback to invalidate cache/monitor.
     *  \param on_restart  Callback to perform the restart (launch new process).
     *  \return reconcile_result indicating what happened.
     */
    reconcile_result check_and_reconcile
    (
        sooperlooper_process_supervisor & supervisor,
        std::function<void(std::uint64_t)> on_invalidate_generation,
        std::function<bool()> on_restart,
        long long now_ms = 0
    );

    /**
     *  Handle shutdown during startup.
     *
     *  If the reconciler is in watching state and shutdown is requested
     *  before the startup grace period expires, cancels any pending
     *  restart and transitions to idle.
     *
     *  \param supervisor  Reference to the process supervisor.
     *  \return true if shutdown was handled.
     */
    bool handle_shutdown_during_startup
    (
        sooperlooper_process_supervisor & supervisor
    );

    /**
     *  Get the current reconciler state.
     */
    reconciler_state state () const;

    /**
     *  Get the number of restart attempts since the last stable interval.
     */
    int restart_count () const;

    /**
     *  Get the current backoff delay in milliseconds.
     */
    int current_backoff_ms () const;

    /**
     *  Get the generation being watched.
     */
    std::uint64_t watched_generation () const;

    /**
     *  Get the list of pending operations.
     */
    const std::vector<pending_operation> & pending_operations () const;

    /**
     *  Get the count of pending operations.
     */
    int pending_count () const;

    /**
     *  Force a state transition (for testing).
     */
    void force_state (reconciler_state s);

    /**
     *  Force a restart count (for testing).
     */
    void force_restart_count (int count);

    /**
     *  Set the last-stable timestamp (for testing stable-interval reset).
     */
    void set_last_stable_time (long long now_ms);

    /**
     *  Check if backoff has elapsed since the last crash.
     *
     *  \param now_ms  Current time in milliseconds.
     *  \return true if backoff has elapsed and restart can proceed.
     */
    bool backoff_elapsed (long long now_ms) const;

    /**
     *  Record the time of the last crash (for backoff calculation).
     */
    void record_crash_time (long long now_ms);

private:
    /** Compute backoff delay for the given restart count. */
    int compute_backoff_ms (int count) const;

    /** Check if the stable interval has elapsed since last crash. */
    bool stable_interval_elapsed (long long now_ms) const;

    reconciler_config m_config;
    reconciler_state m_state{reconciler_state::idle};
    std::uint64_t m_watched_generation{0};
    int m_restart_count{0};
    long long m_last_crash_time_ms{0};
    long long m_last_stable_time_ms{0};
    bool m_startup_grace_active{false};
    long long m_launch_time_ms{0};
    std::vector<pending_operation> m_pending;
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_CRASH_RECONCILER_HPP

/*
 * sooperlooper_crash_reconciler.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
