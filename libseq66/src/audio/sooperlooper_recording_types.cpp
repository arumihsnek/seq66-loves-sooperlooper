/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_recording_types.cpp
 *
 *  Recording-domain type implementations.
 */

#include "audio/sooperlooper_recording_types.hpp"

namespace seq66
{

const char *
recording_intention::type_label () const
{
    switch (intention_type)
    {
        case type::arm:    return "arm";
        case type::begin:  return "begin";
        case type::end:    return "end";
        case type::verify: return "verify";
        case type::cancel: return "cancel";
        default:           return "unknown";
    }
}

const char *
to_string (recording_state s)
{
    switch (s)
    {
        case recording_state::idle:          return "idle";
        case recording_state::armed:         return "armed";
        case recording_state::waiting:       return "waiting";
        case recording_state::recording:     return "recording";
        case recording_state::verifying:     return "verifying";
        case recording_state::complete:      return "complete";
        case recording_state::failed:        return "failed";
        case recording_state::indeterminate: return "indeterminate";
        default:                             return "unknown";
    }
}

const char *
to_string (recording_termination t)
{
    switch (t)
    {
        case recording_termination::completed_exactly:   return "completed_exactly";
        case recording_termination::manually_truncated:  return "manually_truncated";
        case recording_termination::invalidated:         return "invalidated";
        case recording_termination::command_failed:      return "command_failed";
        case recording_termination::verification_failed: return "verification_failed";
        case recording_termination::indeterminate:       return "indeterminate";
        default:                                         return "unknown";
    }
}

const char *
to_string (start_boundary b)
{
    switch (b)
    {
        case start_boundary::next_bar:   return "next_bar";
        case start_boundary::next_beat:  return "next_beat";
        case start_boundary::immediate:  return "immediate";
        default:                         return "unknown";
    }
}

const char *
to_string (tempo_change_policy p)
{
    switch (p)
    {
        case tempo_change_policy::preserve_target_beats:
            return "preserve_target_beats";
        default:
            return "unknown";
    }
}

} // namespace seq66

/*
 * sooperlooper_recording_types.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
