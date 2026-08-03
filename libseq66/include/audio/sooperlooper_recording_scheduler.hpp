#if ! defined SEQ66_SOOPERLOOPER_RECORDING_SCHEDULER_HPP
#define SEQ66_SOOPERLOOPER_RECORDING_SCHEDULER_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_recording_scheduler.hpp
 *
 *  Recording scheduler state machine for exact N-bar musical recording.
 *
 *  The scheduler outputs typed musical intention.  It does NOT send
 *  OSC directly.  The connection to the command dispatcher is a
 *  separate unit.
 *
 *  Dependencies are injected so the scheduler can be tested without
 *  real transport, clock, or dispatch infrastructure.
 */

#include "audio/sooperlooper_recording_types.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace seq66
{

// ======================================================================
//  Injected dependency interfaces
// ======================================================================

/**
 *  Transport state provider — supplies Seq66 transport snapshots.
 *
 *  The scheduler uses ONLY musical state (ticks, beats, bars) for
 *  duration calculations.  Monotonic clock is for deadlines only.
 */
class transport_state_provider
{
public:
    virtual ~transport_state_provider () = default;

    /** Current transport tick position. */
    virtual tick_position current_tick () const = 0;

    /** Current transport generation. */
    virtual transport_generation current_generation () const = 0;

    /** Is transport running? */
    virtual bool is_running () const = 0;

    /** Capture a full metric snapshot from transport. */
    virtual musical_metric capture_metric () const = 0;
};

/**
 *  Monotonic clock provider — used for deadlines and diagnostics only.
 *  NEVER used for musical length definition.
 */
class monotonic_clock_provider
{
public:
    virtual ~monotonic_clock_provider () = default;

    /** Current monotonic time in milliseconds. */
    virtual int64_t now_ms () const = 0;
};

/**
 *  Command result callback — invoked when a command is confirmed or
 *  rejected by the observer layer.
 */
struct command_result
{
    uint64_t request_id{0};
    transport_generation generation;
    bool confirmed{false};
    bool rejected{false};
    std::string reason;
};

/**
 *  Transport observation — what the scheduler sees from the observer.
 */
struct transport_observation
{
    tick_position tick;
    transport_generation generation;
    bool running{false};
    int64_t monotonic_ms{0};
};

// ======================================================================
//  Recording scheduler
// ======================================================================

/**
 *  Recording scheduler — manages the lifecycle of an exact N-bar
 *  musical recording.
 *
 *  The scheduler:
 *  - Accepts a recording_request and calculates a recording_plan
 *  - Emits recording_intentions (arm, begin, end, verify, cancel)
 *  - Tracks state transitions with generation awareness
 *  - Handles manual stop, timeouts, and command results
 *  - Outputs typed musical intention (no OSC)
 *
 *  Thread safety: the scheduler is single-threaded.  The caller
 *  must serialize access.
 */
class recording_scheduler
{
public:
    /** Callback type for emitting intentions. */
    using intention_callback = std::function<void(const recording_intention &)>;

    /** Callback type for state change notification. */
    using state_callback = std::function<void(recording_state, const std::string &)>;

    /**
     *  Construct with injected dependencies.
     *
     *  \param transport   Transport state provider.
     *  \param clock       Monotonic clock provider.
     *  \param tolerance   Recording tolerance configuration.
     */
    recording_scheduler (
        const transport_state_provider & transport,
        const monotonic_clock_provider & clock,
        const recording_tolerance & tolerance = recording_tolerance());

    ~recording_scheduler () = default;

    // -- Configuration ---------------------------------------------------

    /** Set the callback for emitting intentions. */
    void set_intention_callback (intention_callback cb);

    /** Set the callback for state change notifications. */
    void set_state_callback (state_callback cb);

    // -- Request lifecycle ------------------------------------------------

    /**
     *  Start a recording request.  Validates the request and
     *  transitions to armed state.
     *
     *  \return true if the request was accepted, false if rejected.
     */
    bool start_recording (const recording_request & req);

    /**
     *  Cancel the current recording.  Transitions to failed state
     *  with termination = invalidated.
     */
    void cancel_recording (const std::string & reason);

    /**
     *  Manual stop.  Transitions to verifying with termination
     *  = manually_truncated.
     */
    void manual_stop ();

    // -- Observation processing -------------------------------------------

    /**
     *  Process a new transport observation.  This is the main
     *  update loop — called whenever new transport state is available.
     *
     *  Handles:
     *  - Armed → waiting (on boundary detection)
     *  - Waiting → recording (on start boundary)
     *  - Recording → verifying (on stop boundary)
     *  - Generation change (invalidation)
     *  - Timeout detection
     */
    void on_transport_observation (const transport_observation & obs);

    /**
     *  Process a command result from the dispatcher/observer.
     */
    void on_command_result (const command_result & result);

    // -- State inspection -------------------------------------------------

    /** Current scheduler state. */
    recording_state state () const { return m_state; }

    /** Current recording plan (valid only when state >= waiting). */
    const recording_plan & plan () const { return m_plan; }

    /** Current recording request. */
    const recording_request & request () const { return m_request; }

    /** Current termination type (valid in terminal states). */
    recording_termination termination () const { return m_termination; }

    /** Last verification result (valid in terminal states). */
    const verification_result & last_verification () const { return m_verification; }

    /** Current state as a human-readable string. */
    const char * state_string () const;

    /** Is the scheduler idle? */
    bool is_idle () const { return m_state == recording_state::idle; }

    /** Is the scheduler in a terminal state? */
    bool is_terminal () const;

private:
    // -- Internal state machine -------------------------------------------

    void transition_to (recording_state new_state, const std::string & reason);
    bool calculate_plan (const transport_observation & obs);
    void emit_intention (const recording_intention & intent);
    void check_generation_change (const transport_observation & obs);
    void check_timeout (int64_t now_ms);
    void invalidate_plan (const std::string & reason);

    // -- Dependencies (non-owning) ----------------------------------------

    const transport_state_provider & m_transport;
    const monotonic_clock_provider & m_clock;

    // -- Configuration ----------------------------------------------------

    recording_tolerance m_tolerance;

    // -- Callbacks --------------------------------------------------------

    intention_callback m_intention_cb;
    state_callback m_state_cb;

    // -- State ------------------------------------------------------------

    recording_state m_state{recording_state::idle};
    recording_request m_request;
    recording_plan m_plan;
    recording_termination m_termination{recording_termination::indeterminate};
    verification_result m_verification;

    /** Monotonic time when we entered the current state. */
    int64_t m_state_entry_ms{0};

    /** Whether we have emitted the arm intention. */
    bool m_arm_emitted{false};

    /** Whether we have emitted the begin intention. */
    bool m_begin_emitted{false};

    /** Whether we have emitted the end intention. */
    bool m_end_emitted{false};

    /** Last observed transport generation. */
    transport_generation m_last_generation;

    /** Whether we have received a start confirmation. */
    bool m_start_confirmed{false};

    /** Whether we have received a stop confirmation. */
    bool m_stop_confirmed{false};
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_RECORDING_SCHEDULER_HPP

/*
 * sooperlooper_recording_scheduler.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
