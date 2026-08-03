#if ! defined SEQ66_SOOPERLOOPER_RECORDING_TYPES_HPP
#define SEQ66_SOOPERLOOPER_RECORDING_TYPES_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_recording_types.hpp
 *
 *  Recording-domain value types for exact N-bar musical recording.
 *
 *  Seq66 is the musical authority.  Duration is defined in ticks, beats,
 *  and bars derived from Seq66 transport snapshots.  The monotonic clock
 *  is used only for deadlines, diagnostics, and tolerances — never as
 *  the primary definition of musical length.
 *
 *  Design principles:
 *  - Strong integral wrappers prevent unit confusion
 *  - All arithmetic is overflow-checked
 *  - Plans are immutable once calculated
 *  - Generation changes safely invalidate active plans
 *  - Command send != confirmation
 *  - Manual Stop is explicitly truncated, not exact completion
 */

#include <cstdint>
#include <string>
#include <limits>

namespace seq66
{

/* ================================================================== */
/*  Strong integral wrappers                                           */
/* ================================================================== */

/**
 *  A tick position in the Seq66 transport timeline.
 *
 *  Wraps a 64-bit signed integer to prevent unit confusion.
 *  Positions are absolute within the transport.
 */
struct tick_position
{
    int64_t value{0};

    tick_position () = default;
    explicit tick_position (int64_t v) : value(v) {}

    bool operator== (const tick_position & o) const { return value == o.value; }
    bool operator!= (const tick_position & o) const { return value != o.value; }
    bool operator<  (const tick_position & o) const { return value < o.value; }
    bool operator<= (const tick_position & o) const { return value <= o.value; }
    bool operator>  (const tick_position & o) const { return value > o.value; }
    bool operator>= (const tick_position & o) const { return value >= o.value; }

    tick_position operator+ (int64_t offset) const { return tick_position(value + offset); }
    tick_position operator- (int64_t offset) const { return tick_position(value - offset); }
    int64_t operator- (const tick_position & o) const { return value - o.value; }

    bool is_valid () const { return value >= 0; }
};

/**
 *  A count of ticks (duration).
 */
struct tick_count
{
    int64_t value{0};

    tick_count () = default;
    explicit tick_count (int64_t v) : value(v) {}

    bool operator== (const tick_count & o) const { return value == o.value; }
    bool operator!= (const tick_count & o) const { return value != o.value; }
    bool is_positive () const { return value > 0; }
    bool is_valid () const { return value >= 0; }

    /**
     *  Overflow-checked multiplication.
     *  Returns false if the result would overflow.
     */
    bool multiply (int64_t factor, tick_count & result) const
    {
        if (factor <= 0 || value <= 0)
            return false;
        if (value > std::numeric_limits<int64_t>::max() / factor)
            return false;
        result.value = value * factor;
        return true;
    }
};

/**
 *  A count of beats.
 */
struct beat_count
{
    int64_t value{0};

    beat_count () = default;
    explicit beat_count (int64_t v) : value(v) {}

    bool operator== (const beat_count & o) const { return value == o.value; }
    bool is_positive () const { return value > 0; }
};

/**
 *  A count of bars.
 */
struct bar_count
{
    int64_t value{0};

    bar_count () = default;
    explicit bar_count (int64_t v) : value(v) {}

    bool operator== (const bar_count & o) const { return value == o.value; }
    bool is_positive () const { return value > 0; }
};

/**
 *  Transport generation token.
 */
struct transport_generation
{
    uint64_t value{0};

    transport_generation () = default;
    explicit transport_generation (uint64_t v) : value(v) {}

    bool operator== (const transport_generation & o) const { return value == o.value; }
    bool operator!= (const transport_generation & o) const { return value != o.value; }
};

// ======================================================================
//  Recording request

/**
 *  Quantization/start boundary.
 */
enum class start_boundary : int
{
    next_bar,       /**< Start at the next bar boundary.             */
    next_beat,      /**< Start at the next beat boundary.            */
    immediate       /**< Start immediately (no quantization).         */
};

/**
 *  Tempo change policy during recording.
 */
enum class tempo_change_policy : int
{
    preserve_target_beats  /**< Tempo changes alter wall-clock time but
                                never the musical stop position.        */
};

/**
 *  Recording request — what the user wants to record.
 *
 *  Immutable once created.  Contains the musical intent but no
 *  calculated plan or OSC representation.
 */
struct recording_request
{
    uint64_t request_id{0};             /**< Unique request identifier.   */
    bar_count bars{4};                  /**< Number of bars to record.    */
    start_boundary boundary{start_boundary::next_bar};
    tempo_change_policy tempo_policy{tempo_change_policy::preserve_target_beats};
    std::string clip_uuid;              /**< Stable clip identity.        */
    int loop_index{-1};                 /**< Target loop index.           */

    bool is_valid () const
    {
        return bars.is_positive() && loop_index >= 0;
    }
};

// ======================================================================
//  Musical metric snapshot

/**
 *  Immutable musical metric captured from Seq66 transport at a point
 *  in time.  This is the authoritative source for recording duration
 *  calculations.
 */
struct musical_metric
{
    int64_t ticks_per_quarter{480};     /**< Ticks per quarter note.     */
    int numerator{4};                    /**< Time signature numerator.    */
    int denominator{4};                  /**< Time signature denominator.  */
    int64_t ticks_per_beat{480};        /**< Ticks per beat.              */
    int beats_per_bar{4};               /**< Beats per bar.              */
    int64_t start_tempo_bpm{120};       /**< BPM at capture (diagnostic). */
    transport_generation generation;     /**< Transport generation.        */

    /**
     *  Calculate musical duration in ticks for a given bar count.
     *  Uses only Seq66 musical state, never monotonic clock.
     *
     *  \param bars  Number of bars.
     *  \param[out] result  Duration in ticks.
     *  \return true on success, false on overflow or invalid metric.
     */
    bool duration_ticks (bar_count bars, tick_count & result) const
    {
        if (! bars.is_positive() || beats_per_bar <= 0 || ticks_per_beat <= 0)
            return false;

        tick_count beats;
        if (! tick_count(beats_per_bar).multiply(bars.value, beats))
            return false;

        return beats.multiply(ticks_per_beat, result);
    }

    /**
     *  Calculate target beat count for a given bar count.
     */
    bool target_beats (bar_count bars, beat_count & result) const
    {
        if (! bars.is_positive() || beats_per_bar <= 0)
            return false;
        result.value = bars.value * beats_per_bar;
        return result.is_positive();
    }
};

// ======================================================================
//  Recording plan (immutable once calculated)

/**
 *  Calculated recording plan — the immutable target derived from
 *  request + captured metric + transport snapshot.
 */
struct recording_plan
{
    uint64_t request_id{0};             /**< From request.               */
    musical_metric metric;              /**< Captured at arm time.       */
    transport_generation generation;    /**< Transport generation.        */
    tick_position arm_tick;             /**< Transport tick at arm.       */
    tick_position start_tick;           /**< Exact start tick (boundary). */
    bar_count target_bars{0};           /**< From request.               */
    beat_count target_beats{0};         /**< Calculated: bars * bpb.     */
    tick_count duration_ticks{0};       /**< Calculated: beats * tpb.    */
    tick_position stop_tick_exclusive;  /**< start + duration.           */
    std::string clip_uuid;              /**< Stable clip identity.        */
    int loop_index{-1};                 /**< Target loop index.           */

    bool is_valid () const
    {
        return target_bars.is_positive() &&
               target_beats.is_positive() &&
               duration_ticks.is_positive() &&
               start_tick < stop_tick_exclusive;
    }
};

// ======================================================================
//  Scheduler intentions (typed musical commands)

/**
 *  Recording state machine.
 */
enum class recording_state : int
{
    idle,           /**< No recording activity.                      */
    armed,          /**< Valid request accepted, arm intention issued.*/
    waiting,        /**< Arm confirmed, plan calculated, awaiting start.*/
    recording,      /**< Start confirmed, recording in progress.     */
    verifying,      /**< Stop issued, awaiting confirmation.         */
    complete,       /**< Exact N-bar completion verified.            */
    failed,         /**< Recording failed.                           */
    indeterminate   /**< Deadline expired, state unknown.            */
};

/**
 *  Recording termination type.
 */
enum class recording_termination : int
{
    completed_exactly,      /**< N complete bars verified.             */
    manually_truncated,     /**< Manual Stop before N bars.            */
    invalidated,            /**< Generation change invalidated plan.   */
    command_failed,         /**< Command failed or rejected.           */
    verification_failed,    /**< Verification did not confirm.         */
    indeterminate           /**< Ambiguous or timeout.                 */
};

/**
 *  Recording intention — outbound musical command from scheduler.
 *
 *  These are typed intentions that the scheduler emits.  They are NOT
 *  OSC messages and do NOT constitute confirmation.  The dispatcher
 *  sends them; the model observes confirmation separately.
 */
struct recording_intention
{
    enum class type : int
    {
        arm,
        begin,
        end,
        verify,
        cancel
    };

    type intention_type{type::arm};
    uint64_t request_id{0};
    transport_generation generation;
    tick_position tick;             /**< Relevant transport tick.       */
    std::string reason;             /**< Diagnostic reason.            */

    static recording_intention make_arm (uint64_t req_id,
                                         transport_generation gen,
                                         tick_position tick)
    {
        recording_intention i;
        i.intention_type = type::arm;
        i.request_id = req_id;
        i.generation = gen;
        i.tick = tick;
        return i;
    }

    static recording_intention make_begin (uint64_t req_id,
                                           transport_generation gen,
                                           tick_position tick)
    {
        recording_intention i;
        i.intention_type = type::begin;
        i.request_id = req_id;
        i.generation = gen;
        i.tick = tick;
        return i;
    }

    static recording_intention make_end (uint64_t req_id,
                                         transport_generation gen,
                                         tick_position tick)
    {
        recording_intention i;
        i.intention_type = type::end;
        i.request_id = req_id;
        i.generation = gen;
        i.tick = tick;
        return i;
    }

    static recording_intention make_cancel (uint64_t req_id,
                                            transport_generation gen,
                                            const std::string & reason)
    {
        recording_intention i;
        i.intention_type = type::cancel;
        i.request_id = req_id;
        i.generation = gen;
        i.reason = reason;
        return i;
    }

    const char * type_label () const;
};

// ======================================================================
//  Tolerance and verification

/**
 *  Recording tolerance — how much deviation is acceptable.
 *
 *  Musical tolerance is in ticks (typically zero for scheduling).
 *  Monotonic tolerance is in milliseconds (for deadlines only).
 */
struct recording_tolerance
{
    int64_t musical_tolerance_ticks{0}; /**< Allowed early/late in ticks.*/
    int64_t deadline_timeout_ms{5000};  /**< Monotonic deadline.         */
    int64_t observation_timeout_ms{2000}; /**< Max wait for observation. */
};

/**
 *  Verification failure reason.
 */
enum class verification_failure_reason : int
{
    early_start,
    late_start,
    early_stop,
    late_stop,
    metric_mismatch,
    command_rejection,
    contradictory_evidence,
    missing_confirmation,
    timeout,
    generation_loss,
    insufficient_observations
};

/**
 *  Verification result — outcome of recording verification.
 */
struct verification_result
{
    enum class status : int
    {
        verified,
        failed,
        indeterminate
    };

    status result_status{status::indeterminate};
    verification_failure_reason failure_reason{
        verification_failure_reason::missing_confirmation};
    tick_position observed_start_tick;
    tick_position observed_stop_tick;
    int64_t start_tick_error{0};     /**< observed - planned.           */
    int64_t stop_tick_error{0};      /**< observed - planned.           */
    beat_count observed_beats{0};
    tick_count observed_ticks{0};
    recording_termination termination{recording_termination::indeterminate};
    std::string detail;              /**< Human-readable explanation.   */

    bool is_verified () const { return result_status == status::verified; }
    bool is_failed () const { return result_status == status::failed; }
    bool is_indeterminate () const { return result_status == status::indeterminate; }
};

// ======================================================================
//  Free functions

const char * to_string (recording_state s);
const char * to_string (recording_termination t);
const char * to_string (start_boundary b);
const char * to_string (tempo_change_policy p);

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_RECORDING_TYPES_HPP

/*
 * sooperlooper_recording_types.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
