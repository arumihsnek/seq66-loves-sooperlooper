#if ! defined SEQ66_SOOPERLOOPER_AUDIO_SLOT_STATE_RENDERER_HPP
#define SEQ66_SOOPERLOOPER_AUDIO_SLOT_STATE_RENDERER_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_slot_state_renderer.hpp
 *
 *  Audio slot state renderer — derives display state from model snapshots.
 *
 *  This module provides:
 *  - State rendering: pending, stale, offline, failed/error
 *  - Cached meter display
 *  - No synchronous OSC from paint (reads only from model snapshots)
 *  - Bounded update frequency
 *  - Tests with snapshots and absent/stale values
 *
 *  Design principles:
 *  - All rendering reads from model snapshots, never from engine directly
 *  - No synchronous OSC or network calls from paint/event handlers
 *  - Update frequency is bounded by the caller
 *  - Absent/stale values are distinguished from zero
 */

#include <cstdint>
#include <string>
#include "audio/sooperlooper_audio_slot_widget.hpp"

namespace seq66
{

/**
 *  Display state derived from the model.
 */
enum class display_state : int
{
    idle,           /**< No activity.                               */
    pending,        /**< Action sent, awaiting confirmation.        */
    active,         /**< Engine in active state (record/play/...).  */
    stale,          /**< Feedback has stopped.                       */
    offline,        /**< Engine is unreachable.                      */
    failed,         /**< Action failed or error.                     */
    indeterminate   /**< Deadline expired, state unknown.           */
};

/**
 *  Cached meter value.
 */
struct cached_meter
{
    float in_peak{0.0f};
    float out_peak{0.0f};
    bool present{false};
    uint64_t timestamp_ms{0};

    bool is_fresh (uint64_t now_ms, uint64_t max_age_ms = 5000) const
    {
        return present && (now_ms - timestamp_ms) < max_age_ms;
    }
};

/**
 *  State renderer result.
 */
struct render_state
{
    display_state display{display_state::idle};
    cached_meter meter;
    std::string state_text;
    std::string command_text;
    bool transport_enabled{false};
    bool record_enabled{false};

    /** Human-readable label for the display state. */
    const char * display_label () const;
};

/**
 *  State renderer.
 *
 *  Derives display state from model snapshots.  Stateless — reads
 *  the model and produces a render result without side effects.
 */
class sooperlooper_audio_slot_state_renderer
{
public:
    sooperlooper_audio_slot_state_renderer () = default;
    ~sooperlooper_audio_slot_state_renderer () = default;

    /**
     *  Render the display state from the model.
     *
     *  \param model    Current audio slot model snapshot.
     *  \param now_ms   Current time in milliseconds.
     *  \return Rendered display state.
     */
    render_state render (const audio_slot_model & model,
                         uint64_t now_ms) const;

    /**
     *  Quick check — is the display state stale?
     */
    bool is_stale (const audio_slot_model & model,
                   uint64_t now_ms,
                   uint64_t stale_threshold_ms = 5000) const;

    /**
     *  Quick check — is the display state failed/error?
     */
    bool is_failed (const audio_slot_model & model) const;
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_AUDIO_SLOT_STATE_RENDERER_HPP

/*
 * sooperlooper_audio_slot_state_renderer.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
