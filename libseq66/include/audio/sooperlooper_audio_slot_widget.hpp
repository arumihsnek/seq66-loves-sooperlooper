#if ! defined SEQ66_SOOPERLOOPER_AUDIO_SLOT_WIDGET_HPP
#define SEQ66_SOOPERLOOPER_AUDIO_SLOT_WIDGET_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_slot_widget.hpp
 *
 *  Audio slot widget — model layer.
 *
 *  This file defines the state model and action interface for an audio
 *  slot widget.  It deliberately contains NO Qt dependencies so that
 *  the model can be unit-tested without a display server.  A separate
 *  Qt widget class will own a reference to this model and render it.
 *
 *  Design principles:
 *  - Sending is NOT confirmation.  Every outbound action goes through
 *    the command dispatcher; the model observes feedback separately.
 *  - The model is a plain data container with pure-logic transitions.
 *  - No visual decisions are encoded here.  Colours, fonts, sizes
 *    belong in the presentation layer.
 *
 *  \todo M3-004-VISUAL: L3 visual/UX design decision required once the
 *  model widget is executable and reviewable.
 */

#include <cstdint>
#include <string>

namespace seq66
{

/**
 *  Loop state as observed from the engine.
 *
 *  Mirrors the sooperlooper engine state integers documented in
 *  OSC-CONTROL-AND-FEEDBACK.md.
 */
enum class loop_state
{
    off,
    wait,               /**< Waiting for a quantized trigger.    */
    record,
    play,
    overdub,
    multiply,
    insert,
    replace,
    revert,
    pause,
    scratch,
    reverse,
    one_shot,
    tripped,
    stop,               /**< Explicit stop requested.            */
    unknown
};

/**
 *  Command status as observed from the dispatcher.
 */
enum class command_feedback
{
    none,               /**< No active command.                  */
    pending,            /**< Command sent, awaiting confirmation. */
    confirmed,          /**< Engine confirmed the operation.      */
    failed,             /**< Command failed or timed out.         */
    indeterminate       /**< Deadline expired, state unknown.     */
};

/**
 *  Tempo mode selection.
 */
enum class tempo_mode_selection
{
    free,
    tape,
    elastic
};

/**
 *  Transport action.
 */
enum class transport_action
{
    start,
    stop
};

/**
 *  Audio slot model state.
 *
 *  Pure data; no side effects, no Qt, no I/O.
 */
struct audio_slot_model
{
    /** Runtime loop index from the mapper. */
    int runtime_index{-1};

    /** Clip UUID.  Empty if no clip is assigned. */
    std::string clip_uuid;

    /** Observed loop state from the engine. */
    loop_state observed_state{loop_state::off};

    /** Command feedback from the dispatcher. */
    command_feedback command_status{command_feedback::none};

    /** Current tempo mode selection. */
    tempo_mode_selection tempo_mode{tempo_mode_selection::free};

    /** Whether transport is playing. */
    bool transport_playing{false};

    /** Error message when command_status == failed. */
    std::string last_error;

    /** Timestamp (ms since epoch) of the last state change. */
    uint64_t last_state_change_ms{0};

    /* ---------------------------------------------------------- */
    /*  Derived helpers (pure functions, no mutation).             */
    /* ---------------------------------------------------------- */

    /**
     *  Is a clip assigned to this slot?
     */
    bool has_clip () const { return ! clip_uuid.empty(); }

    /**
     *  Is the command status a terminal state (no more feedback expected)?
     */
    bool command_terminal () const
    {
        return command_status == command_feedback::confirmed ||
               command_status == command_feedback::failed ||
               command_status == command_feedback::indeterminate;
    }

    /**
     *  Human-readable label for the observed state.
     */
    const char * state_label () const;

    /**
     *  Human-readable label for the command feedback.
     */
    const char * command_label () const;

    /**
     *  Human-readable label for the tempo mode.
     */
    const char * tempo_label () const;
};

/**
 *  Audio slot action requests.
 *
 *  These are outbound requests that the widget emits.  They are NOT
 *  confirmed by the model; confirmation comes from feedback.
 */
struct audio_slot_action
{
    transport_action transport{transport_action::start};
    tempo_mode_selection tempo{tempo_mode_selection::free};
    bool has_transport{false};
    bool has_tempo{false};

    static audio_slot_action make_transport (transport_action a)
    {
        audio_slot_action act;
        act.transport = a;
        act.has_transport = true;
        return act;
    }

    static audio_slot_action make_tempo (tempo_mode_selection m)
    {
        audio_slot_action act;
        act.tempo = m;
        act.has_tempo = true;
        return act;
    }

    static audio_slot_action make_both (
        transport_action a,
        tempo_mode_selection m)
    {
        audio_slot_action act;
        act.transport = a;
        act.tempo = m;
        act.has_transport = true;
        act.has_tempo = true;
        return act;
    }
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_AUDIO_SLOT_WIDGET_HPP

/*
 * sooperlooper_audio_slot_widget.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
