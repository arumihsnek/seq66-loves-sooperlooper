/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_slot_state_renderer.cpp
 *
 *  State renderer implementation.
 */

#include "audio/sooperlooper_audio_slot_state_renderer.hpp"

namespace seq66
{

render_state
sooperlooper_audio_slot_state_renderer::render (
    const audio_slot_model & model,
    uint64_t now_ms) const
{
    render_state result;

    /* Derive display state from model */
    if (model.command_status == command_feedback::pending)
    {
        result.display = display_state::pending;
    }
    else if (model.command_status == command_feedback::failed)
    {
        result.display = display_state::failed;
    }
    else if (model.command_status == command_feedback::indeterminate)
    {
        result.display = display_state::indeterminate;
    }
    else if (model.observed_state == loop_state::off)
    {
        result.display = display_state::idle;
    }
    else if (model.observed_state == loop_state::unknown)
    {
        /* Unknown state could indicate offline or error */
        result.display = display_state::offline;
    }
    else
    {
        /* Active state (record, play, overdub, etc.) */
        result.display = display_state::active;
    }

    /* Check for staleness */
    if (model.last_state_change_ms > 0 &&
        (now_ms - model.last_state_change_ms) > 5000)
    {
        if (result.display == display_state::active)
            result.display = display_state::stale;
    }

    /* Labels */
    result.state_text = model.state_label();
    result.command_text = model.command_label();

    /* Enabled states */
    result.transport_enabled = !model.transport_playing;
    result.record_enabled = model.has_clip() && !model.transport_playing;

    /* Meter placeholder — real meter would come from engine feedback */
    result.meter.present = false;

    return result;
}

bool
sooperlooper_audio_slot_state_renderer::is_stale (
    const audio_slot_model & model,
    uint64_t now_ms,
    uint64_t stale_threshold_ms) const
{
    if (model.last_state_change_ms == 0)
        return false;
    return (now_ms - model.last_state_change_ms) > stale_threshold_ms;
}

bool
sooperlooper_audio_slot_state_renderer::is_failed (
    const audio_slot_model & model) const
{
    return model.command_status == command_feedback::failed ||
           model.observed_state == loop_state::unknown;
}

const char *
render_state::display_label () const
{
    switch (display)
    {
        case display_state::idle:          return "idle";
        case display_state::pending:       return "pending";
        case display_state::active:        return "active";
        case display_state::stale:         return "stale";
        case display_state::offline:       return "offline";
        case display_state::failed:        return "failed";
        case display_state::indeterminate: return "indeterminate";
        default:                           return "unknown";
    }
}

} // namespace seq66

/*
 * sooperlooper_audio_slot_state_renderer.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
