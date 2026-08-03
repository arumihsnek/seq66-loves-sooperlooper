#if ! defined SEQ66_SOOPERLOOPER_RECORDING_VERIFIER_HPP
#define SEQ66_SOOPERLOOPER_RECORDING_VERIFIER_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_recording_verifier.hpp
 *
 *  Recording length verifier — checks observed recording length
 *  against the calculated plan.
 *
 *  The verifier compares:
 *  - Expected duration in ticks
 *  - Expected start/stop positions
 *  - Tolerance (musical and monotonic)
 *
 *  It does NOT send commands; it only produces verification_result.
 */

#include "audio/sooperlooper_recording_types.hpp"

namespace seq66
{

/**
 *  Recording verifier — validates observed recording length against plan.
 */
class recording_verifier
{
public:
    recording_verifier () = default;

    /**
     *  Verify a recording against the planned length.
     *
     *  \param plan          The recording plan to verify against.
     *  \param observed_start Actual start tick from observer.
     *  \param observed_stop  Actual stop tick from observer.
     *  \param tolerance     Tolerance configuration.
     *  \return verification_result
     */
    static verification_result verify_length (
        const recording_plan & plan,
        const tick_position & observed_start,
        const tick_position & observed_stop,
        const recording_tolerance & tolerance = recording_tolerance());

    /**
     *  Verify with observed beat count (if available from SooperLooper).
     */
    static verification_result verify_beats (
        const recording_plan & plan,
        const beat_count & observed_beats,
        const recording_tolerance & tolerance = recording_tolerance());
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_RECORDING_VERIFIER_HPP
