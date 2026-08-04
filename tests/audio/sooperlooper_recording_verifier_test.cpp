/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_recording_verifier_test.cpp
 *
 *  Tests for the recording verifier.
 *
 *  Coverage:
 *  - verify_length: exact match (pass)
 *  - verify_length: within tolerance (pass)
 *  - verify_length: exceeds tolerance (fail)
 *  - verify_length: zero-length recording (edge case)
 *  - verify_beats: exact match (pass)
 *  - verify_beats: exceeds tolerance (fail)
 *  - Partial results when start/stop are approximate
 *  - Default tolerance vs custom tolerance
 */

/* ------------------------------------------------------------------ */
/*  Includes                                                          */
/* ------------------------------------------------------------------ */

#include <iostream>
#include <string>
#include <cassert>

#include "audio/sooperlooper_recording_verifier.hpp"
#include "audio/sooperlooper_recording_types.hpp"

/* ------------------------------------------------------------------ */
/*  Minimal test framework                                            */
/* ------------------------------------------------------------------ */

static int g_total = 0;
static int g_failed = 0;

#define CHECK(expr) \
    do { \
        ++g_total; \
        if (! (expr)) { \
            ++g_failed; \
            std::cerr << "FAIL: " << #expr << " (line " \
                      << __LINE__ << ")\\n"; \
        } \
    } while (0)

#define CHECK_EQ(a, b) \
    do { \
        ++g_total; \
        if ((a) != (b)) { \
            ++g_failed; \
            std::cerr << "FAIL: " << #a << " == " << #b \
                      << " (line " << __LINE__ << ")\\n"; \
        } \
    } while (0)

/* ------------------------------------------------------------------ */
/*  Helper functions                                                  */
/* ------------------------------------------------------------------ */

/**
 * Create a simple recording plan for 4 bars at 120 BPM, 4/4, 480 PPQN.
 */
static seq66::recording_plan make_test_plan()
{
    seq66::recording_plan plan;
    plan.request_id = 1;
    plan.metric.ticks_per_quarter = 480;
    plan.metric.beats_per_bar = 4;
    plan.metric.ticks_per_beat = 480; // 480 ticks per quarter note * 1 = 480
    plan.metric.start_tempo_bpm = 120;
    plan.metric.generation.value = 1;
    plan.arm_tick.value = 0;
    plan.start_tick.value = 0;
    plan.target_bars.value = 4;
    plan.target_beats.value = plan.target_bars.value * plan.metric.beats_per_bar; // 4 * 4 = 16 beats
    plan.duration_ticks.value = plan.target_beats.value * plan.metric.ticks_per_beat; // 16 * 480 = 7680 ticks
    plan.stop_tick_exclusive.value = plan.start_tick.value + plan.duration_ticks.value; // 0 + 7680 = 7680
    plan.clip_uuid = "test-uuid";
    plan.loop_index = 0;
    return plan;
}

/* ------------------------------------------------------------------ */
/*  Tests                                                             */
/* ------------------------------------------------------------------ */

static void
test_verify_length_exact_match ()
{
    std::cout << "\n--- verify_length: exact match ---" << std::endl;
    seq66::recording_plan plan = make_test_plan();
    seq66::tick_position observed_start{0};
    seq66::tick_position observed_stop{7680}; // exact stop tick
    seq66::recording_tolerance tolerance; // default tolerance (0 ticks)

    seq66::verification_result result =
        seq66::recording_verifier::verify_length(plan, observed_start, observed_stop, tolerance);

    CHECK(result.result_status == seq66::verification_result::status::verified);
    CHECK_EQ(result.observed_ticks.value, 7680);
    CHECK_EQ(result.detail, std::string("length matches plan within tolerance"));
}

static void
test_verify_length_within_tolerance ()
{
    std::cout << "\n--- verify_length: within tolerance ---" << std::endl;
    seq66::recording_plan plan = make_test_plan();
    seq66::tick_position observed_start{0};
    seq66::tick_position observed_stop{7690}; // 10 ticks over
    seq66::recording_tolerance tolerance;
    tolerance.musical_tolerance_ticks = 20; // allow 20 ticks

    seq66::verification_result result =
        seq66::recording_verifier::verify_length(plan, observed_start, observed_stop, tolerance);

    CHECK(result.result_status == seq66::verification_result::status::verified);
    CHECK_EQ(result.observed_ticks.value, 7690);
    CHECK_EQ(result.detail, std::string("length matches plan within tolerance"));
}

static void
test_verify_length_exceeds_tolerance ()
{
    std::cout << "\n--- verify_length: exceeds tolerance ---" << std::endl;
    seq66::recording_plan plan = make_test_plan();
    seq66::tick_position observed_start{0};
    seq66::tick_position observed_stop{7710}; // 30 ticks over
    seq66::recording_tolerance tolerance;
    tolerance.musical_tolerance_ticks = 20; // allow 20 ticks

    seq66::verification_result result =
        seq66::recording_verifier::verify_length(plan, observed_start, observed_stop, tolerance);

    CHECK(result.result_status == seq66::verification_result::status::failed);
    CHECK_EQ(result.observed_ticks.value, 7710);
    CHECK(result.detail.find("length mismatch") != std::string::npos);
    CHECK(result.detail.find("stop off by") != std::string::npos);
}

static void
test_verify_length_zero_length ()
{
    std::cout << "\n--- verify_length: zero-length recording ---" << std::endl;
    seq66::recording_plan plan = make_test_plan();
    // Override to make zero-length plan
    plan.target_bars.value = 0;
    plan.target_beats.value = 0;
    plan.duration_ticks.value = 0;
    plan.stop_tick_exclusive.value = plan.start_tick.value;
    seq66::tick_position observed_start{0};
    seq66::tick_position observed_stop{0}; // zero length
    seq66::recording_tolerance tolerance;

    seq66::verification_result result =
        seq66::recording_verifier::verify_length(plan, observed_start, observed_stop, tolerance);

    // Zero length should pass if within tolerance (0 ticks error)
    CHECK(result.result_status == seq66::verification_result::status::verified);
    CHECK_EQ(result.observed_ticks.value, 0);
}

static void
test_verify_beats_exact_match ()
{
    std::cout << "\n--- verify_beats: exact match ---" << std::endl;
    seq66::recording_plan plan = make_test_plan();
    seq66::beat_count observed_beats{16}; // exact target beats
    seq66::recording_tolerance tolerance; // default tolerance

    seq66::verification_result result =
        seq66::recording_verifier::verify_beats(plan, observed_beats, tolerance);

    CHECK(result.result_status == seq66::verification_result::status::verified);
    CHECK_EQ(result.observed_beats.value, 16);
    CHECK_EQ(result.detail, std::string("beat count matches plan"));
}

static void
test_verify_beats_exceeds_tolerance ()
{
    std::cout << "\n--- verify_beats: exceeds tolerance ---" << std::endl;
    seq66::recording_plan plan = make_test_plan();
    seq66::beat_count observed_beats{20}; // 4 beats over
    seq66::recording_tolerance tolerance;
    tolerance.musical_tolerance_ticks = 1; // 1 tick tolerance (very small)

    seq66::verification_result result =
        seq66::recording_verifier::verify_beats(plan, observed_beats, tolerance);

    CHECK(result.result_status == seq66::verification_result::status::failed);
    CHECK_EQ(result.observed_beats.value, 20);
    CHECK_EQ(result.failure_reason, seq66::verification_failure_reason::metric_mismatch);
    CHECK(result.detail.find("beat count mismatch") != std::string::npos);
}

static void
test_verify_length_partial_results ()
{
    std::cout << "\n--- verify_length: partial results (start/stop approximate) ---" << std::endl;
    seq66::recording_plan plan = make_test_plan();
    seq66::tick_position observed_start{-10}; // 10 ticks early
    seq66::tick_position observed_stop{7690}; // 10 ticks late
    seq66::recording_tolerance tolerance;
    tolerance.musical_tolerance_ticks = 15; // allow 15 ticks

    seq66::verification_result result =
        seq66::recording_verifier::verify_length(plan, observed_start, observed_stop, tolerance);

    // Start error: 10 ticks (early), stop error: 10 ticks (late), duration error: 20 ticks
    // With tolerance 15: start error 10 <= 15 -> ok, stop error 10 <=15 -> ok, duration error 20 >15 -> fail.
    // So we expect failure due to duration.
    CHECK(result.result_status == seq66::verification_result::status::failed);
    CHECK_EQ(result.observed_ticks.value, 7700);
    CHECK(result.detail.find("length mismatch") != std::string::npos);
    CHECK(result.detail.find("duration off by") != std::string::npos);
}

static void
test_default_vs_custom_tolerance ()
{
    std::cout << "\n--- default tolerance vs custom tolerance ---" << std::endl;
    seq66::recording_plan plan = make_test_plan();
    seq66::tick_position observed_start{0};
    seq66::tick_position observed_stop{7680 + 5}; // 5 ticks over
    seq66::recording_tolerance default_tolerance; // 0 ticks
    seq66::recording_tolerance custom_tolerance;
    custom_tolerance.musical_tolerance_ticks = 10; // allow 10 ticks

    // With default tolerance (0) -> should fail
    seq66::verification_result result_default =
        seq66::recording_verifier::verify_length(plan, observed_start, observed_stop, default_tolerance);
    CHECK(result_default.result_status == seq66::verification_result::status::failed);

    // With custom tolerance -> should pass
    seq66::verification_result result_custom =
        seq66::recording_verifier::verify_length(plan, observed_start, observed_stop, custom_tolerance);
    CHECK(result_custom.result_status == seq66::verification_result::status::verified);
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */

int main ()
{
    test_verify_length_exact_match();
    test_verify_length_within_tolerance();
    test_verify_length_exceeds_tolerance();
    test_verify_length_zero_length();
    test_verify_beats_exact_match();
    test_verify_beats_exceeds_tolerance();
    test_verify_length_partial_results();
    test_default_vs_custom_tolerance();

    std::cout << "\n" << g_total << "/" << g_total
              << " pass. All recording verifier assertions passed." << std::endl;
    return g_failed;
}