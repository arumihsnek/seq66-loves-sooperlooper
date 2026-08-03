#if ! defined SEQ66_SOOPERLOOPER_AUDIO_SLOT_CONTROLS_HPP
#define SEQ66_SOOPERLOOPER_AUDIO_SLOT_CONTROLS_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_audio_slot_controls.hpp
 *
 *  Audio slot controls — connects widget actions to command dispatcher.
 *
 *  This module provides:
 *  - Record, Launch, Mute, Overdub, Stop actions
 *  - Connection to the command dispatcher
 *  - Send != confirmation (dispatch sends, model observes feedback)
 *  - pending/confirmed/failed/indeterminate from observed feedback
 *  - Timeout and error visibility
 *
 *  Design principles:
 *  - Actions are outbound requests; confirmation comes from feedback
 *  - The controls module is a thin adapter, not a deep modification
 *  - All operations are non-blocking and use the dispatcher
 */

#include <string>
#include <cstdint>
#include "audio/sooperlooper_audio_slot_widget.hpp"

namespace seq66
{

/**
 *  Audio slot action type.
 */
enum class audio_slot_action_type : int
{
    record,
    launch,
    mute,
    overdub,
    stop
};

/**
 *  Audio slot action request.
 *
 *  Outbound request that the controls module emits.  The dispatcher
 *  sends it; the model observes confirmation separately.
 */
struct audio_slot_control_request
{
    audio_slot_action_type type{audio_slot_action_type::record};
    int loop_index{-1};
    std::string clip_uuid;
    uint64_t timestamp_ms{0};

    static audio_slot_control_request make_record (int idx, const std::string & uuid)
    {
        audio_slot_control_request r;
        r.type = audio_slot_action_type::record;
        r.loop_index = idx;
        r.clip_uuid = uuid;
        r.timestamp_ms = current_time_ms();
        return r;
    }

    static audio_slot_control_request make_launch (int idx, const std::string & uuid)
    {
        audio_slot_control_request r;
        r.type = audio_slot_action_type::launch;
        r.loop_index = idx;
        r.clip_uuid = uuid;
        r.timestamp_ms = current_time_ms();
        return r;
    }

    static audio_slot_control_request make_mute (int idx, const std::string & uuid)
    {
        audio_slot_control_request r;
        r.type = audio_slot_action_type::mute;
        r.loop_index = idx;
        r.clip_uuid = uuid;
        r.timestamp_ms = current_time_ms();
        return r;
    }

    static audio_slot_control_request make_overdub (int idx, const std::string & uuid)
    {
        audio_slot_control_request r;
        r.type = audio_slot_action_type::overdub;
        r.loop_index = idx;
        r.clip_uuid = uuid;
        r.timestamp_ms = current_time_ms();
        return r;
    }

    static audio_slot_control_request make_stop (int idx, const std::string & uuid)
    {
        audio_slot_control_request r;
        r.type = audio_slot_action_type::stop;
        r.loop_index = idx;
        r.clip_uuid = uuid;
        r.timestamp_ms = current_time_ms();
        return r;
    }

    /** Human-readable label for the action type. */
    const char * type_label () const;

private:
    static uint64_t current_time_ms ();
};

/**
 *  Audio slot controls.
 *
 *  Thin adapter that converts user actions into dispatcher requests.
 *  The controls module does NOT confirm actions — confirmation comes
 *  from observed feedback via the command tracker.
 */
class sooperlooper_audio_slot_controls
{
public:
    sooperlooper_audio_slot_controls () = default;
    ~sooperlooper_audio_slot_controls () = default;

    /**
     *  Request a record action.
     *
     *  \param loop_index  Loop index in the engine.
     *  \param clip_uuid   Clip UUID for stable identity.
     *  \return The control request (for dispatcher).
     */
    audio_slot_control_request request_record (int loop_index,
                                               const std::string & clip_uuid);

    /**
     *  Request a launch action.
     */
    audio_slot_control_request request_launch (int loop_index,
                                               const std::string & clip_uuid);

    /**
     *  Request a mute action.
     */
    audio_slot_control_request request_mute (int loop_index,
                                             const std::string & clip_uuid);

    /**
     *  Request an overdub action.
     */
    audio_slot_control_request request_overdub (int loop_index,
                                                const std::string & clip_uuid);

    /**
     *  Request a stop action.
     */
    audio_slot_control_request request_stop (int loop_index,
                                             const std::string & clip_uuid);

    /**
     *  Convert a transport_action to a control request.
     */
    audio_slot_control_request from_transport (transport_action action,
                                               int loop_index,
                                               const std::string & clip_uuid);
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_AUDIO_SLOT_CONTROLS_HPP

/*
 * sooperlooper_audio_slot_controls.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
