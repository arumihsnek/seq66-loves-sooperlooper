/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_recording_scheduler.cpp
 *
 *  Recording scheduler state machine implementation.
 */

#include "audio/sooperlooper_recording_scheduler.hpp"

#include <algorithm>
#include <limits>

namespace seq66
{

// ======================================================================
//  Construction
// ======================================================================

recording_scheduler::recording_scheduler (
    const transport_state_provider & transport,
    const monotonic_clock_provider & clock,
    const recording_tolerance & tolerance)
    : m_transport(transport)
    , m_clock(clock)
    , m_tolerance(tolerance)
{
}

// ======================================================================
//  Configuration
// ======================================================================

void
recording_scheduler::set_intention_callback (intention_callback cb)
{
    m_intention_cb = std::move(cb);
}

void
recording_scheduler::set_state_callback (state_callback cb)
{
    m_state_cb = std::move(cb);
}

// ======================================================================
//  Request lifecycle
// ======================================================================

bool
recording_scheduler::start_recording (const recording_request & req)
{
    if (m_state != recording_state::idle)
        return false;

    if (! req.is_valid())
        return false;

    /* Validate metric: beats_per_bar and ticks_per_beat must be positive. */
    musical_metric metric = m_transport.capture_metric();
    if (metric.beats_per_bar <= 0 || metric.ticks_per_beat <= 0)
        return false;

    m_request = req;
    m_arm_emitted = false;
    m_begin_emitted = false;
    m_end_emitted = false;
    m_start_confirmed = false;
    m_stop_confirmed = false;
    m_termination = recording_termination::indeterminate;

    transition_to(recording_state::armed, "valid request accepted");

    /* Emit arm intention. */
    tick_position current = m_transport.current_tick();
    transport_generation gen = m_transport.current_generation();
    m_plan.request_id = req.request_id;
    m_plan.metric = metric;
    m_plan.generation = gen;
    m_plan.arm_tick = current;

    emit_intention(recording_intention::make_arm(req.request_id, gen, current));
    m_arm_emitted = true;

    return true;
}

void
recording_scheduler::cancel_recording (const std::string & reason)
{
    if (is_terminal() || m_state == recording_state::idle)
        return;

    invalidate_plan(reason);
    m_termination = recording_termination::invalidated;
    transition_to(recording_state::failed, reason);
}

void
recording_scheduler::manual_stop ()
{
    if (m_state != recording_state::recording)
        return;

    /* Emit end intention for manual stop. */
    tick_position current = m_transport.current_tick();
    transport_generation gen = m_transport.current_generation();
    emit_intention(recording_intention::make_end(m_request.request_id, gen, current));
    m_end_emitted = true;

    m_termination = recording_termination::manually_truncated;
    transition_to(recording_state::verifying, "manual stop");
}

// ======================================================================
//  Observation processing
// ======================================================================

void
recording_scheduler::on_transport_observation (const transport_observation & obs)
{
    if (is_terminal() || m_state == recording_state::idle)
        return;

    /* Check generation change. */
    check_generation_change(obs);

    /* Check timeout. */
    check_timeout(obs.monotonic_ms);

    /* Update last generation. */
    m_last_generation = obs.generation;

    /* Process based on current state. */
    switch (m_state)
    {
        case recording_state::armed:
        {
            /* Waiting for the next bar boundary to calculate plan. */
            if (obs.running && obs.tick.value > m_plan.arm_tick.value)
            {
                if (calculate_plan(obs))
                {
                    transition_to(recording_state::waiting, "plan calculated");
                }
            }
            break;
        }

        case recording_state::waiting:
        {
            /* Waiting for the start boundary. */
            if (obs.tick.value >= m_plan.start_tick.value)
            {
                m_start_confirmed = true;
                emit_intention(recording_intention::make_begin(
                    m_request.request_id, obs.generation, m_plan.start_tick));
                m_begin_emitted = true;
                transition_to(recording_state::recording, "start boundary reached");
            }
            break;
        }

        case recording_state::recording:
        {
            /* Check if we've reached the stop boundary. */
            if (obs.tick.value >= m_plan.stop_tick_exclusive.value)
            {
                m_stop_confirmed = true;
                emit_intention(recording_intention::make_end(
                    m_request.request_id, obs.generation, m_plan.stop_tick_exclusive));
                m_end_emitted = true;
                m_termination = recording_termination::completed_exactly;
                transition_to(recording_state::verifying, "stop boundary reached");
            }
            break;
        }

        case recording_state::verifying:
        {
            /* Waiting for verification.  Check timeout. */
            int64_t elapsed = obs.monotonic_ms - m_state_entry_ms;
            if (elapsed >= m_tolerance.observation_timeout_ms)
            {
                m_verification.result_status =
                    verification_result::status::indeterminate;
                m_verification.failure_reason =
                    verification_failure_reason::timeout;
                m_verification.detail = "verification timeout";
                m_termination = recording_termination::indeterminate;
                transition_to(recording_state::indeterminate, "verification timeout");
            }
            break;
        }

        default:
            break;
    }
}

void
recording_scheduler::on_command_result (const command_result & result)
{
    if (is_terminal() || m_state == recording_state::idle)
        return;

    /* Ignore results from wrong generation. */
    if (result.generation != m_plan.generation)
        return;

    /* Ignore results for wrong request. */
    if (result.request_id != m_request.request_id)
        return;

    if (result.rejected)
    {
        /* Command rejected — fail. */
        m_verification.result_status = verification_result::status::failed;
        m_verification.failure_reason =
            verification_failure_reason::command_rejection;
        m_verification.detail = result.reason;
        m_termination = recording_termination::command_failed;
        transition_to(recording_state::failed, "command rejected: " + result.reason);
    }
    else if (result.confirmed)
    {
        /* Record confirmation based on current state. */
        if (m_state == recording_state::verifying && m_end_emitted)
        {
            /* Stop confirmed. */
            m_verification.result_status = verification_result::status::verified;
            m_verification.observed_start_tick = m_plan.start_tick;
            m_verification.observed_stop_tick = m_plan.stop_tick_exclusive;
            m_verification.start_tick_error = 0;
            m_verification.stop_tick_error = 0;
            m_verification.observed_beats = m_plan.target_beats;
            m_verification.observed_ticks = m_plan.duration_ticks;
            m_verification.termination = m_termination;

            if (m_termination == recording_termination::completed_exactly)
                transition_to(recording_state::complete, "exact completion verified");
            else
                transition_to(recording_state::complete,
                    "truncated completion verified");
        }
    }
}

// ======================================================================
//  State inspection
// ======================================================================

const char *
recording_scheduler::state_string () const
{
    return to_string(m_state);
}

bool
recording_scheduler::is_terminal () const
{
    return m_state == recording_state::complete ||
           m_state == recording_state::failed ||
           m_state == recording_state::indeterminate;
}

// ======================================================================
//  Internal state machine
// ======================================================================

void
recording_scheduler::transition_to (
    recording_state new_state,
    const std::string & reason)
{
    m_state = new_state;
    m_state_entry_ms = m_clock.now_ms();

    if (m_state_cb)
        m_state_cb(new_state, reason);
}

bool
recording_scheduler::calculate_plan (const transport_observation & obs)
{
    musical_metric metric = m_transport.capture_metric();

    tick_count duration;
    if (! metric.duration_ticks(m_request.bars, duration))
        return false;

    beat_count beats;
    if (! metric.target_beats(m_request.bars, beats))
        return false;

    /* Calculate start tick based on boundary. */
    tick_position start;
    switch (m_request.boundary)
    {
        case start_boundary::next_bar:
        {
            /* Round up to next bar boundary. */
            int64_t ticks_per_bar = metric.ticks_per_beat * metric.beats_per_bar;
            if (ticks_per_bar <= 0)
                return false;
            int64_t pos = obs.tick.value;
            int64_t remainder = pos % ticks_per_bar;
            if (remainder == 0 && pos > obs.tick.value)
                start = tick_position(pos);
            else
                start = tick_position(pos + ticks_per_bar - remainder);
            break;
        }

        case start_boundary::next_beat:
        {
            int64_t pos = obs.tick.value;
            int64_t remainder = pos % metric.ticks_per_beat;
            if (remainder == 0)
                start = tick_position(pos + metric.ticks_per_beat);
            else
                start = tick_position(pos + metric.ticks_per_beat - remainder);
            break;
        }

        case start_boundary::immediate:
            start = obs.tick;
            break;
    }

    tick_position stop = tick_position(start.value + duration.value);

    m_plan.request_id = m_request.request_id;
    m_plan.metric = metric;
    m_plan.generation = obs.generation;
    m_plan.start_tick = start;
    m_plan.target_bars = m_request.bars;
    m_plan.target_beats = beats;
    m_plan.duration_ticks = duration;
    m_plan.stop_tick_exclusive = stop;
    m_plan.clip_uuid = m_request.clip_uuid;
    m_plan.loop_index = m_request.loop_index;

    return m_plan.is_valid();
}

void
recording_scheduler::emit_intention (const recording_intention & intent)
{
    if (m_intention_cb)
        m_intention_cb(intent);
}

void
recording_scheduler::check_generation_change (const transport_observation & obs)
{
    if (obs.generation != m_plan.generation && m_state != recording_state::armed)
    {
        invalidate_plan("generation change");
        m_termination = recording_termination::invalidated;
        transition_to(recording_state::failed, "generation change");
    }
}

void
recording_scheduler::check_timeout (int64_t now_ms)
{
    if (m_state == recording_state::armed)
    {
        int64_t elapsed = now_ms - m_state_entry_ms;
        if (elapsed >= m_tolerance.deadline_timeout_ms)
        {
            invalidate_plan("armed timeout");
            m_termination = recording_termination::indeterminate;
            transition_to(recording_state::indeterminate, "armed timeout");
        }
    }
}

void
recording_scheduler::invalidate_plan (const std::string & reason)
{
    /* Cancel any pending intentions. */
    if (m_arm_emitted && ! m_begin_emitted)
    {
        emit_intention(recording_intention::make_cancel(
            m_request.request_id, m_plan.generation, reason));
    }
}

} // namespace seq66

/*
 * sooperlooper_recording_scheduler.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
