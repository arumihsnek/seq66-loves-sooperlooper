#if ! defined SEQ66_SOOPERLOOPER_RECORDING_ORCHESTRATOR_HPP
#define SEQ66_SOOPERLOOPER_RECORDING_ORCHESTRATOR_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_recording_orchestrator.hpp
 *
 *  Orchestrator: bridges recording scheduler → command dispatcher.
 *
 *  The orchestrator:
 *  - Receives typed recording_intention from the scheduler
 *  - Translates to command_outbound for the existing dispatcher
 *  - Feeds command results back to the scheduler
 *  - Tracks command lifecycle (pending, confirmed, rejected)
 */

#include "audio/sooperlooper_recording_scheduler.hpp"

#include <unordered_map>

namespace seq66
{

/**
 *  Command outbound — what the orchestrator sends to the dispatcher.
 */
struct command_outbound
{
    enum class action
    {
        set_loop,
        set_command,
        hit
    };

    action type{action::set_loop};
    int loop_index{-1};
    std::string command;
    std::string value;
    uint64_t request_id{0};
    transport_generation generation;
};

/**
 *  Command dispatch interface — abstract for testability.
 */
class command_dispatch_interface
{
public:
    virtual ~command_dispatch_interface () = default;
    virtual bool dispatch (const command_outbound & cmd) = 0;
};

/**
 *  Recording orchestrator — bridges scheduler intentions to
 *  command dispatch.
 */
class recording_orchestrator
{
public:
    recording_orchestrator (
        recording_scheduler & scheduler,
        command_dispatch_interface & dispatch);

    ~recording_orchestrator () = default;

    /** Start a recording (delegates to scheduler). */
    bool start_recording (const recording_request & req);

    /** Cancel the current recording. */
    void cancel_recording (const std::string & reason);

    /** Manual stop. */
    void manual_stop ();

    /** Process transport observation. */
    void on_transport_observation (const transport_observation & obs);

    /** Process a command result from the dispatcher. */
    void on_command_result (const command_result & result);

    /** Current scheduler state. */
    recording_state state () const { return m_scheduler.state(); }

private:
    void on_intention (const recording_intention & intent);

    recording_scheduler & m_scheduler;
    command_dispatch_interface & m_dispatch;

    /** Map request_id → outbound command for tracking. */
    std::unordered_map<uint64_t, command_outbound> m_pending;
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_RECORDING_ORCHESTRATOR_HPP
