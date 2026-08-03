/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_recording_verifier.cpp
 *
 *  Recording length verifier implementation.
 */

#include "audio/sooperlooper_recording_verifier.hpp"

#include <algorithm>
#include <cstdlib>

namespace seq66
{

verification_result
recording_verifier::verify_length (
    const recording_plan & plan,
    const tick_position & observed_start,
    const tick_position & observed_stop,
    const recording_tolerance & tolerance)
{
    verification_result vr;
    vr.observed_start_tick = observed_start;
    vr.observed_stop_tick = observed_stop;

    /* Calculate errors. */
    int64_t start_error = std::abs(observed_start.value - plan.start_tick.value);
    int64_t stop_error = std::abs(observed_stop.value - plan.stop_tick_exclusive.value);
    int64_t duration_error = std::abs(
        (observed_stop.value - observed_start.value) - plan.duration_ticks.value);

    vr.start_tick_error = start_error;
    vr.stop_tick_error = stop_error;
    vr.observed_ticks = tick_count(static_cast<int64_t>(
        observed_stop.value - observed_start.value));

    /* Check musical tolerance (in ticks). */
    bool start_ok = start_error <= tolerance.musical_tolerance_ticks;
    bool stop_ok = stop_error <= tolerance.musical_tolerance_ticks;
    bool duration_ok = duration_error <= tolerance.musical_tolerance_ticks;

    if (start_ok && stop_ok && duration_ok)
    {
        vr.result_status = verification_result::status::verified;
        vr.failure_reason = verification_failure_reason::missing_confirmation;
        vr.detail = "length matches plan within tolerance";
    }
    else
    {
        vr.result_status = verification_result::status::failed;
        vr.detail = "length mismatch";

        if (! start_ok)
        {
            vr.failure_reason = (start_error > 0) ?
                verification_failure_reason::late_start :
                verification_failure_reason::early_start;
            vr.detail += "; start off by " + std::to_string(start_error) + " ticks";
        }
        else if (! stop_ok)
        {
            vr.failure_reason = (stop_error > 0) ?
                verification_failure_reason::late_stop :
                verification_failure_reason::early_stop;
            vr.detail += "; stop off by " + std::to_string(stop_error) + " ticks";
        }
        else
        {
            vr.failure_reason = verification_failure_reason::metric_mismatch;
            vr.detail += "; duration off by " + std::to_string(duration_error) + " ticks";
        }
    }

    return vr;
}

verification_result
recording_verifier::verify_beats (
    const recording_plan & plan,
    const beat_count & observed_beats,
    const recording_tolerance & tolerance)
{
    verification_result vr;
    vr.observed_beats = observed_beats;

    int64_t beat_error = std::abs(observed_beats.value - plan.target_beats.value);
    vr.observed_ticks = tick_count(observed_beats.value *
        plan.metric.ticks_per_beat);

    if (beat_error <= tolerance.musical_tolerance_ticks)
    {
        vr.result_status = verification_result::status::verified;
        vr.failure_reason = verification_failure_reason::missing_confirmation;
        vr.detail = "beat count matches plan";
    }
    else
    {
        vr.result_status = verification_result::status::failed;
        vr.failure_reason = verification_failure_reason::metric_mismatch;
        vr.detail = "beat count mismatch: expected " +
            std::to_string(plan.target_beats.value) +
            ", observed " + std::to_string(observed_beats.value);
    }

    return vr;
}

} // namespace seq66

/*
 * sooperlooper_recording_verifier.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
