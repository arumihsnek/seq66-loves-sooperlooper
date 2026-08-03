/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_slot_controls.cpp
 *
 *  Audio slot controls implementation.
 */

#include "audio/sooperlooper_audio_slot_controls.hpp"

#include <chrono>

namespace seq66
{

uint64_t
audio_slot_control_request::current_time_ms ()
{
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return static_cast<uint64_t>(ms);
}

const char *
audio_slot_control_request::type_label () const
{
    switch (type)
    {
        case audio_slot_action_type::record:   return "record";
        case audio_slot_action_type::launch:   return "launch";
        case audio_slot_action_type::mute:     return "mute";
        case audio_slot_action_type::overdub:  return "overdub";
        case audio_slot_action_type::stop:     return "stop";
        default:                              return "unknown";
    }
}

audio_slot_control_request
sooperlooper_audio_slot_controls::request_record (int loop_index,
                                                  const std::string & clip_uuid)
{
    return audio_slot_control_request::make_record(loop_index, clip_uuid);
}

audio_slot_control_request
sooperlooper_audio_slot_controls::request_launch (int loop_index,
                                                  const std::string & clip_uuid)
{
    return audio_slot_control_request::make_launch(loop_index, clip_uuid);
}

audio_slot_control_request
sooperlooper_audio_slot_controls::request_mute (int loop_index,
                                                const std::string & clip_uuid)
{
    return audio_slot_control_request::make_mute(loop_index, clip_uuid);
}

audio_slot_control_request
sooperlooper_audio_slot_controls::request_overdub (int loop_index,
                                                   const std::string & clip_uuid)
{
    return audio_slot_control_request::make_overdub(loop_index, clip_uuid);
}

audio_slot_control_request
sooperlooper_audio_slot_controls::request_stop (int loop_index,
                                                const std::string & clip_uuid)
{
    return audio_slot_control_request::make_stop(loop_index, clip_uuid);
}

audio_slot_control_request
sooperlooper_audio_slot_controls::from_transport (transport_action action,
                                                  int loop_index,
                                                  const std::string & clip_uuid)
{
    switch (action)
    {
        case transport_action::start:
            return request_launch(loop_index, clip_uuid);
        case transport_action::stop:
            return request_stop(loop_index, clip_uuid);
        default:
            return request_stop(loop_index, clip_uuid);
    }
}

} // namespace seq66

/*
 * sooperlooper_audio_slot_controls.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
