#if ! defined SEQ66_SOOPERLOOPER_PROCESS_SUPERVISOR_HPP
#define SEQ66_SOOPERLOOPER_PROCESS_SUPERVISOR_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_process_supervisor.hpp
 *
 *  Managed process supervisor for a headless SooperLooper engine.
 *
 *  Seq66 launches SooperLooper as an owned child process, verifies its
 *  identity by PID, tracks its lifecycle, and performs bounded shutdown
 *  escalation.  The supervisor never kills externally discovered processes.
 *
 *  The process adapter is injectable: callers provide a system_interface so
 *  that tests can supply fake implementations without forking real processes.
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace seq66
{

/**
 *  Supervisor lifecycle state.
 *
 *  State machine:
 *      idle -> starting -> running -> stopping -> idle
 *                          |           |
 *                          v           v
 *                       failed      failed
 *                          |
 *                          v
 *                       cooldown -> starting
 */
enum class supervisor_state : int
{
    idle,               /**< No process launched or after clean shutdown.  */
    starting,           /**< Process forked, waiting for identity check.  */
    running,            /**< Owned child verified and alive.              */
    stopping,           /**< Graceful shutdown initiated (TERM sent).     */
    failed,             /**< Process exited unexpectedly or kill sent.    */
    cooldown            /**< Brief pause before automatic restart.        */
};

/**
 *  Result of a launch attempt.
 */
enum class launch_result : int
{
    success,            /**< Child launched and identity verified.         */
    already_running,    /**< A child is already managed.                  */
    fork_failed,        /**< OS refused to create a child process.        */
    exec_failed,        /**< Child could not exec the executable.         */
    identity_mismatch,  /**< Child PID does not match expected.           */
    backend_blocked     /**< Backend probe says unavailable.              */
};

/**
 *  Shutdown escalation level.
 */
enum class shutdown_level : int
{
    graceful,           /**< Send /quit via OSC.                          */
    terminate,          /**< Send SIGTERM.                                */
    kill                /**< Send SIGKILL.                                */
};

/**
 *  Process adapter interface.
 *
 *  Injectable abstraction over OS process operations.  The production
 *  implementation uses fork/exec/waitpid/signals.  Tests inject a fake
 *  that records calls and returns predetermined results.
 */
class process_adapter
{
public:
    virtual ~process_adapter () = default;

    /**
     *  Fork and exec a child process.
     *
     *  \param argv        Null-terminated argument vector (argv[0] = program).
     *  \param[out] child_pid  PID of the child on success.
     *  \return true on success, false if fork/exec failed.
     */
    virtual bool spawn
    (
        const std::vector<std::string> & argv,
        int & child_pid
    ) = 0;

    /**
     *  Send a signal to a specific PID.
     *
     *  \param pid         Target process.
     *  \param signal      Signal number (SIGTERM, SIGKILL, etc.).
     *  \return true if the signal was sent.
     */
    virtual bool send_signal (int pid, int signal) = 0;

    /**
     *  Check whether a process with the given PID is still alive.
     */
    virtual bool is_alive (int pid) = 0;

    /**
     *  Wait for a process to exit, up to timeout_ms.
     *
     *  \param pid         Process to wait for.
     *  \param timeout_ms  Maximum wait time.
     *  \return true if the process exited within the timeout.
     */
    virtual bool wait_exit (int pid, int timeout_ms) = 0;
};

/**
 *  Configuration for the process supervisor.
 */
struct supervisor_config
{
    std::string executable_path;    /**< Path to sooperlooper binary.       */
    std::vector<std::string> args;  /**< Additional arguments.             */
    int graceful_timeout_ms{2000};  /**< Timeout for graceful shutdown.    */
    int term_timeout_ms{3000};      /**< Timeout after SIGTERM before KILL.*/
    int cooldown_ms{1000};          /**< Pause before restart attempt.     */
    int max_restarts{3};            /**< Max automatic restarts.           */
};

/**
 *  Managed process supervisor for a headless SooperLooper engine.
 *
 *  Owns one child process at a time.  Verifies the child's identity by
 *  PID.  Escalates shutdown: graceful (OSC /quit) → SIGTERM → SIGKILL.
 *  Never kills a process it did not spawn.
 *
 *  Thread safety: all methods are safe to call from a single coordinator
 *  thread.  The caller must not call concurrent methods from multiple
 *  threads without external synchronization.
 */
class sooperlooper_process_supervisor
{
public:
    sooperlooper_process_supervisor ();
    explicit sooperlooper_process_supervisor
    (
        std::shared_ptr<process_adapter> adapter
    );
    ~sooperlooper_process_supervisor ();

    sooperlooper_process_supervisor
        (const sooperlooper_process_supervisor &) = delete;
    sooperlooper_process_supervisor & operator =
        (const sooperlooper_process_supervisor &) = delete;

    /**
     *  Update configuration.  Only effective when state is idle or failed.
     */
    void set_config (const supervisor_config & cfg);

    /** Get current configuration. */
    const supervisor_config & get_config () const;

    /**
     *  Launch a new child process.
     *
     *  Increments the generation.  Returns error if a child is already
     *  running.  The launch uses the configured executable and arguments.
     *
     *  \return launch_result indicating success or specific failure.
     */
    launch_result launch ();

    /**
     *  Initiate bounded shutdown of the owned child.
     *
     *  Follows the escalation sequence: graceful → TERM → KILL.
     *  Each phase waits up to the configured timeout before escalating.
     *
     *  \return true if the child was stopped or was already not running.
     */
    bool shutdown ();

    /**
     *  Force immediate termination (SIGKILL) of the owned child.
     *
     *  Only sends the signal to the verified owned child PID.
     */
    void force_kill ();

    /**
     *  Attempt a restart of the owned child.
     *
     *  Shuts down the current child (if running) and launches a new one.
     *  Respects max_restarts limit.
     *
     *  \return launch_result of the new launch attempt.
     */
    launch_result restart ();

    /** Get the current supervisor state. */
    supervisor_state state () const;

    /** Get the current engine generation. */
    std::uint64_t generation () const;

    /** Get the PID of the managed child (0 if none). */
    int child_pid () const;

    /** Return true if the managed child is alive. */
    bool child_alive () const;

    /** Get the last error message. */
    const std::string & last_error () const;

    /** Get the number of restart attempts since last successful launch. */
    int restart_count () const;

    /**
     *  Check whether the managed child has exited.
     *
     *  Non-blocking.  If the child has exited, transitions to failed.
     *  Should be called periodically from the coordinator loop.
     *
     *  \return true if the state changed (child exited unexpectedly).
     */
    bool poll ();

    /**
     *  Force a state transition (for testing or explicit lifecycle control).
     */
    void force_state (supervisor_state s);

    /**
     *  Build the deterministic argument vector for the child process.
     *
     *  Uses the configured executable path and arguments.
     *  The result is suitable for process_adapter::spawn().
     */
    std::vector<std::string> build_argv () const;

private:
    /** Verify that the child PID matches expectations. */
    bool verify_identity (int pid);

    /** Attempt graceful shutdown via OSC /quit. */
    bool graceful_shutdown ();

    /** Escalate to SIGTERM. */
    bool term_shutdown ();

    /** Escalate to SIGKILL. */
    bool kill_shutdown ();

    std::shared_ptr<process_adapter> m_adapter;
    supervisor_config m_config;
    supervisor_state m_state{supervisor_state::idle};
    std::uint64_t m_generation{0};
    int m_child_pid{0};
    int m_expected_pid{0};
    int m_restart_count{0};
    std::string m_last_error;
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_PROCESS_SUPERVISOR_HPP

/*
 * sooperlooper_process_supervisor.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
