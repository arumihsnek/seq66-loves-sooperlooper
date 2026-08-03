/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_recording_orchestrator.cpp
 *
 *  Recording orchestrator implementation.
 */

#include "audio/sooperlooper_recording_orchestrator.hpp"

namespace seq66
{

recording_orchestrator::recording_orchestrator (
    recording_scheduler & scheduler,
    command_dispatch_interface & dispatch)
    : m_scheduler(scheduler)
    , m_dispatch(dispatch)
{
    m_scheduler.set_intention_callback(
        [this](const recording_intention & i) { on_intention(i); });
}

bool
recording_orchestrator::start_recording (const recording_request & req)
{
    return m_scheduler.start_recording(req);
}

void
recording_orchestrator::cancel_recording (const std::string & reason)
{
    m_scheduler.cancel_recording(reason);
}

void
recording_orchestrator::manual_stop ()
{
    m_scheduler.manual_stop();
}

void
recording_orchestrator::on_transport_observation (const transport_observation & obs)
{
    m_scheduler.on_transport_observation(obs);
}

void
recording_orchestrator::on_command_result (const command_result & result)
{
    m_scheduler.on_command_result(result);
}

void
recording_orchestrator::on_intention (const recording_intention & intent)
{
    command_outbound cmd;
    cmd.request_id = intent.request_id;
    cmd.generation = intent.generation;
    /* loop_index comes from the scheduler's plan. */
    cmd.loop_index = m_scheduler.plan().loop_index;

    switch (intent.intention_type)
    {
        case recording_intention::type::arm:
        {
            cmd.type = command_outbound::action::set_loop;
            cmd.command = "record";
            cmd.value = "1";
            break;
        }

        case recording_intention::type::begin:
        {
            cmd.type = command_outbound::action::set_loop;
            cmd.command = "record";
            cmd.value = "1";
            break;
        }

        case recording_intention::type::end:
        {
            cmd.type = command_outbound::action::set_loop;
            cmd.command = "record";
            cmd.value = "0";
            break;
        }

        case recording_intention::type::cancel:
        {
            cmd.type = command_outbound::action::set_loop;
            cmd.command = "record";
            cmd.value = "0";
            break;
        }

        case recording_intention::type::verify:
        {
            cmd.type = command_outbound::action::hit;
            cmd.command = "verify_length";
            break;
        }
    }

    m_pending[cmd.request_id] = cmd;
    m_dispatch.dispatch(cmd);
}

} // namespace seq66

/*
 * sooperlooper_recording_orchestrator.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
