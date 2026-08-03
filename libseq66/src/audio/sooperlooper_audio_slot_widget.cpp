/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_slot_widget.cpp
 *
 *  Audio slot model implementation.
 */

#include "audio/sooperlooper_audio_slot_widget.hpp"

namespace seq66
{

const char *
audio_slot_model::state_label () const
{
    switch (observed_state)
    {
        case loop_state::off:       return "off";
        case loop_state::wait:      return "wait";
        case loop_state::record:    return "record";
        case loop_state::play:      return "play";
        case loop_state::overdub:   return "overdub";
        case loop_state::multiply:  return "multiply";
        case loop_state::insert:    return "insert";
        case loop_state::replace:   return "replace";
        case loop_state::revert:    return "revert";
        case loop_state::pause:     return "pause";
        case loop_state::scratch:   return "scratch";
        case loop_state::reverse:   return "reverse";
        case loop_state::one_shot:  return "one-shot";
        case loop_state::tripped:   return "tripped";
        case loop_state::stop:      return "stop";
        case loop_state::unknown:   return "unknown";
    }
    return "unknown";
}

const char *
audio_slot_model::command_label () const
{
    switch (command_status)
    {
        case command_feedback::none:          return "idle";
        case command_feedback::pending:       return "pending";
        case command_feedback::confirmed:     return "confirmed";
        case command_feedback::failed:        return "failed";
        case command_feedback::indeterminate: return "indeterminate";
    }
    return "unknown";
}

const char *
audio_slot_model::tempo_label () const
{
    switch (tempo_mode)
    {
        case tempo_mode_selection::free:    return "free";
        case tempo_mode_selection::tape:    return "tape";
        case tempo_mode_selection::elastic: return "elastic";
    }
    return "unknown";
}

} // namespace seq66

/*
 * sooperlooper_audio_slot_widget.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
