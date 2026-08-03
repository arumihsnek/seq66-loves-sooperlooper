#if ! defined SEQ66_SOOPERLOOPER_ENGINE_LAUNCHER_HPP
#define SEQ66_SOOPERLOOPER_ENGINE_LAUNCHER_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_engine_launcher.hpp
 *
 *  Engine launch orchestration for headless SooperLooper.
 *
 *  Bridges the backend probe, process supervisor, engine monitor, and
 *  observed-state cache into a coherent lifecycle.  The launcher owns
 *  the launch/reconciliation state machine and ensures generation
 *  synchronization across all components.
 *
 *  Lifecycle ordering (coordinator thread only):
 *    1. Probe backend (must be usable).
 *    2. Compute prospective generation and deterministic names.
 *    3. Launch process via supervisor.
 *    4. Publish generation to observed cache.
 *    5. Reset engine monitor to starting state.
 *    6. Begin asynchronous OSC reconciliation.
 *    7. On shutdown: invalidate callbacks → stop reconciliation →
 *       delegate to supervisor → force offline state.
 *
 *  The launcher does NOT own the OSC client, receiver, or cache.
 *  It holds references/callbacks to coordinate them.
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "audio/sooperlooper_backend_probe.hpp"
#include "audio/sooperlooper_engine_monitor.hpp"
#include "audio/sooperlooper_process_supervisor.hpp"

namespace seq66
{

class sooperlooper_observed_cache;

/**
 *  Launcher lifecycle state.
 *
 *  State machine:
 *      idle -> probing -> naming -> launching -> running -> stopping -> idle
 *                                   |              |
 *                                   v              v
 *                                failed         failed
 *                                                  |
 *                                                  v
 *                                              cooldown
 */
enum class launcher_state : int
{
    idle,           /**< No process managed.                      */
    probing,        /**< Backend capability probe in progress.    */
    naming,         /**< Deterministic names being computed.      */
    launching,      /**< Supervisor.launch() called, waiting.     */
    running,        /**< Engine started, reconciliation active.   */
    stopping,       /**< Shutdown sequence in progress.           */
    failed,         /**< Launch or reconciliation failed.         */
    cooldown        /**< Brief pause before retry.                */
};

/**
 *  Deterministic naming policy.
 *
 *  Derives OSC port and JACK client name from instance identity
 *  and engine generation.  Names are collision-resistant because
 *  they include a hash of (instance_id, generation).
 */
struct engine_naming_policy
{
    std::string instance_id;        /**< Persistent Seq66 instance ID.     */
    int osc_port_base{9951};        /**< Base port for SooperLooper OSC.   */
    int osc_port_range{100};        /**< Range of candidate ports.         */
    std::string jack_client_prefix{"seq66-sl"};  /**< JACK name prefix.   */

    /**
     *  Compute deterministic OSC port from instance_id + generation.
     *  Returns a port within [osc_port_base, osc_port_base + osc_port_range).
     */
    int compute_osc_port (std::uint64_t generation) const;

    /**
     *  Compute deterministic JACK client name from instance_id + generation.
     *  Sanitized for JACK length/character limits.
     */
    std::string compute_jack_client_name (std::uint64_t generation) const;
};

/**
 *  Result of a launch attempt.
 */
enum class launcher_result : int
{
    success,            /**< Engine launched and generation published.  */
    backend_unavailable,/**< Backend probe says no JACK/PipeWire.      */
    probe_error,        /**< Backend probe failed.                     */
    naming_conflict,    /**< Deterministic name reservation failed.    */
    supervisor_failed,  /**< Process supervisor launch failed.         */
    monitor_error       /**< Engine monitor integration failed.        */
};

/**
 *  Adapter interface for launcher dependencies.
 *
 *  Injectable abstraction so tests can supply fake implementations
 *  without real processes, OSC, or JACK.
 */
class engine_launcher_adapter
{
public:
    virtual ~engine_launcher_adapter () = default;

    /** Run the backend capability probe. */
    virtual backend_probe_result probe_backend () = 0;

    /** Check if an OSC port is available (not in use). */
    virtual bool is_port_available (int port) = 0;

    /**
     *  Attempt to reserve/claim an OSC port.
     *  Returns true if the port was successfully reserved.
     */
    virtual bool reserve_port (int port) = 0;

    /** Release a previously reserved port. */
    virtual void release_port (int port) = 0;
};

/**
 *  Engine launch orchestrator.
 *
 *  Owns the launch/reconciliation lifecycle.  Serializes all mutations
 *  on one coordinator thread.  Does NOT own the client, receiver, cache,
 *  supervisor, or monitor — it coordinates them through the adapter
 *  and provided references.
 *
 *  Thread safety: all methods are safe to call from a single
 *  coordinator thread.  No concurrent access assumed.
 */
class sooperlooper_engine_launcher
{
public:
    sooperlooper_engine_launcher ();
    explicit sooperlooper_engine_launcher (
        std::shared_ptr<engine_launcher_adapter> adapter,
        std::shared_ptr<process_adapter> proc_adapter
    );
    ~sooperlooper_engine_launcher ();

    sooperlooper_engine_launcher
        (const sooperlooper_engine_launcher &) = delete;
    sooperlooper_engine_launcher & operator =
        (const sooperlooper_engine_launcher &) = delete;

    /**
     *  Configure the naming policy.  Only effective when idle.
     */
    void set_naming_policy (const engine_naming_policy & policy);

    /** Get current naming policy. */
    const engine_naming_policy & naming_policy () const;

    /**
     *  Configure the process supervisor arguments.
     *  Only effective when idle.
     */
    void set_supervisor_config (const supervisor_config & cfg);

    /**
     *  Configure the engine monitor.  The launcher will call
     *  force_state() and reset() on the monitor during lifecycle.
     */
    void set_engine_monitor (sooperlooper_engine_monitor * monitor);

    /**
     *  Set the observed cache.  The launcher will call
     *  set_generation() on the cache during lifecycle.
     */
    void set_observed_cache (sooperlooper_observed_cache * cache);

    /**
     *  Perform a complete launch sequence.
     *
     *  Ordering:
     *    1. Probe backend (adapter->probe_backend()).
     *    2. Compute prospective generation.
     *    3. Compute and reserve deterministic names.
     *    4. Configure supervisor with names and arguments.
     *    5. Launch via supervisor.
     *    6. Publish generation to observed cache.
     *    7. Reset engine monitor.
     *    8. Transition to running state.
     *
     *  \return launcher_result indicating success or failure.
     */
    launcher_result launch ();

    /**
     *  Initiate shutdown sequence.
     *
     *  Ordering:
     *    1. Transition to stopping.
     *    2. Delegate shutdown to supervisor.
     *    3. Release reserved port.
     *    4. Transition to idle.
     */
    bool shutdown ();

    /**
     *  Get the current launcher state.
     */
    launcher_state state () const;

    /**
     *  Get the current engine generation.
     */
    std::uint64_t generation () const;

    /**
     *  Get the last backend probe result.
     */
    const backend_probe_result & last_probe () const;

    /**
     *  Get the last error message.
     */
    const std::string & last_error () const;

    /**
     *  Get the computed OSC port for the current/superseded generation.
     */
    int current_osc_port () const;

    /**
     *  Get the computed JACK client name for the current/superseded
     *  generation.
     */
    const std::string & current_jack_client_name () const;

    /**
     *  Check if the managed engine is alive.
     */
    bool engine_alive () const;

    /**
     *  Force a state transition (for testing).
     */
    void force_state (launcher_state s);

    /**
     *  Get a reference to the internal process supervisor.
     *  For testing and direct lifecycle queries.
     */
    const sooperlooper_process_supervisor & supervisor () const;

private:
    /** Compute the next generation from the supervisor. */
    std::uint64_t compute_prospective_generation () const;

    /** Reserve deterministic names for a generation. */
    bool reserve_names (std::uint64_t gen);

    /** Release reserved names. */
    void release_names ();

    /** Publish generation to observed cache and reset monitor. */
    void publish_generation (std::uint64_t gen);

    std::shared_ptr<engine_launcher_adapter> m_adapter;
    sooperlooper_process_supervisor m_supervisor;
    engine_naming_policy m_naming_policy;
    sooperlooper_engine_monitor * m_monitor{nullptr};
    sooperlooper_observed_cache * m_cache{nullptr};
    launcher_state m_state{launcher_state::idle};
    std::uint64_t m_generation{0};
    int m_reserved_port{0};
    std::string m_jack_client_name;
    backend_probe_result m_last_probe;
    std::string m_last_error;
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_ENGINE_LAUNCHER_HPP

/*
 * sooperlooper_engine_launcher.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
