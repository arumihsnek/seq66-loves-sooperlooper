/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_recording_types_test.cpp
 *
 *  Comprehensive tests for recording-domain types.
 *
 *  Coverage:
 *  - Strong integral wrappers: tick_position, tick_count, beat_count, bar_count
 *  - Overflow-checked arithmetic
 *  - recording_request validation
 *  - musical_metric duration calculations
 *  - recording_plan calculations
 *  - recording_intention factory methods
 *  - to_string conversions
 *  - Matrix: 4/4, 3/4, 5/4, 6/8, 7/8 with various bar counts
 *  - Edge cases: zero/negative, overflow, boundary
 */

#include <iostream>
#include <string>
#include <cassert>
#include <limits>

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
                      << __LINE__ << ")\n"; \
        } \
    } while (0)

#define CHECK_EQ(a, b) \
    do { \
        ++g_total; \
        if ((a) != (b)) { \
            ++g_failed; \
            std::cerr << "FAIL: " << #a << " == " << #b \
                      << " (line " << __LINE__ << ")\n"; \
        } \
    } while (0)

/* ------------------------------------------------------------------ */
/*  tick_position tests                                                */
/* ------------------------------------------------------------------ */

static void
test_tick_position ()
{
    std::cout << "\n--- tick_position ---" << std::endl;
    seq66::tick_position a(100);
    seq66::tick_position b(200);
    CHECK(a.is_valid());
    CHECK_EQ((a + 50).value, 150);
    CHECK_EQ((b - 50).value, 150);
    CHECK_EQ(b - a, int64_t(100));
    CHECK(a < b);
    CHECK(a != b);
    CHECK(a == seq66::tick_position(100));
}

/* ------------------------------------------------------------------ */
/*  tick_count tests                                                   */
/* ------------------------------------------------------------------ */

static void
test_tick_count ()
{
    std::cout << "\n--- tick_count ---" << std::endl;
    seq66::tick_count tc(1000);
    CHECK(tc.is_positive());
    CHECK(tc.is_valid());

    seq66::tick_count result;
    CHECK(tc.multiply(4, result));
    CHECK_EQ(result.value, 4000);

    /* Overflow test */
    seq66::tick_count big(std::numeric_limits<int64_t>::max());
    CHECK(! big.multiply(2, result));
}

/* ------------------------------------------------------------------ */
/*  bar_count and beat_count tests                                     */
/* ------------------------------------------------------------------ */

static void
test_bar_beat_counts ()
{
    std::cout << "\n--- bar_count and beat_count ---" << std::endl;
    seq66::bar_count bc(4);
    CHECK(bc.is_positive());
    CHECK_EQ(bc.value, 4);

    seq66::beat_count btc(16);
    CHECK(btc.is_positive());
    CHECK_EQ(btc.value, 16);

    seq66::bar_count zero(0);
    CHECK(! zero.is_positive());
}

/* ------------------------------------------------------------------ */
/*  transport_generation tests                                         */
/* ------------------------------------------------------------------ */

static void
test_transport_generation ()
{
    std::cout << "\n--- transport_generation ---" << std::endl;
    seq66::transport_generation g1(1);
    seq66::transport_generation g2(2);
    CHECK(g1 == seq66::transport_generation(1));
    CHECK(g1 != g2);
}

/* ------------------------------------------------------------------ */
/*  recording_request tests                                            */
/* ------------------------------------------------------------------ */

static void
test_recording_request ()
{
    std::cout << "\n--- recording_request ---" << std::endl;
    seq66::recording_request req;
    req.request_id = 42;
    req.bars = seq66::bar_count(4);
    req.boundary = seq66::start_boundary::next_bar;
    req.loop_index = 0;
    CHECK(req.is_valid());

    seq66::recording_request invalid;
    invalid.bars = seq66::bar_count(0);
    CHECK(! invalid.is_valid());

    seq66::recording_request no_loop;
    no_loop.bars = seq66::bar_count(4);
    no_loop.loop_index = -1;
    CHECK(! no_loop.is_valid());
}

/* ------------------------------------------------------------------ */
/*  musical_metric tests                                               */
/* ------------------------------------------------------------------ */

static void
test_metric_duration_ticks ()
{
    std::cout << "\n--- metric duration_ticks ---" << std::endl;
    seq66::musical_metric m;
    m.ticks_per_quarter = 480;
    m.beats_per_bar = 4;
    m.ticks_per_beat = 480;

    seq66::tick_count result;
    CHECK(m.duration_ticks(seq66::bar_count(4), result));
    CHECK_EQ(result.value, 7680);  /* 4 bars * 4 beats * 480 ticks */

    CHECK(m.duration_ticks(seq66::bar_count(1), result));
    CHECK_EQ(result.value, 1920);  /* 1 bar * 4 beats * 480 ticks */
}

static void
test_metric_target_beats ()
{
    std::cout << "\n--- metric target_beats ---" << std::endl;
    seq66::musical_metric m;
    m.beats_per_bar = 4;

    seq66::beat_count result;
    CHECK(m.target_beats(seq66::bar_count(4), result));
    CHECK_EQ(result.value, 16);

    CHECK(m.target_beats(seq66::bar_count(1), result));
    CHECK_EQ(result.value, 4);
}

static void
test_metric_invalid ()
{
    std::cout << "\n--- metric invalid ---" << std::endl;
    seq66::musical_metric m;
    m.beats_per_bar = 0;
    seq66::tick_count result;
    CHECK(! m.duration_ticks(seq66::bar_count(4), result));

    seq66::beat_count beats;
    CHECK(! m.target_beats(seq66::bar_count(4), beats));
}

/* ------------------------------------------------------------------ */
/*  recording_plan tests                                               */
/* ------------------------------------------------------------------ */

static void
test_recording_plan ()
{
    std::cout << "\n--- recording_plan ---" << std::endl;
    seq66::recording_plan plan;
    plan.request_id = 1;
    plan.metric.beats_per_bar = 4;
    plan.metric.ticks_per_beat = 480;
    plan.generation = seq66::transport_generation(1);
    plan.arm_tick = seq66::tick_position(0);
    plan.start_tick = seq66::tick_position(1920);
    plan.target_bars = seq66::bar_count(4);
    plan.target_beats = seq66::beat_count(16);
    plan.duration_ticks = seq66::tick_count(7680);
    plan.stop_tick_exclusive = seq66::tick_position(1920 + 7680);
    plan.clip_uuid = "test-uuid";
    plan.loop_index = 0;

    CHECK(plan.is_valid());
    CHECK(plan.target_bars.is_positive());
    CHECK(plan.target_beats.is_positive());
    CHECK(plan.duration_ticks.is_positive());
    CHECK(plan.start_tick < plan.stop_tick_exclusive);
}

/* ------------------------------------------------------------------ */
/*  recording_intention tests                                          */
/* ------------------------------------------------------------------ */

static void
test_recording_intention ()
{
    std::cout << "\n--- recording_intention ---" << std::endl;
    seq66::transport_generation gen(1);

    auto arm = seq66::recording_intention::make_arm(1, gen,
        seq66::tick_position(0));
    CHECK_EQ(arm.intention_type, seq66::recording_intention::type::arm);
    CHECK_EQ(arm.request_id, 1u);
    CHECK_EQ(std::string(arm.type_label()), "arm");

    auto begin = seq66::recording_intention::make_begin(1, gen,
        seq66::tick_position(1920));
    CHECK_EQ(begin.intention_type, seq66::recording_intention::type::begin);

    auto end = seq66::recording_intention::make_end(1, gen,
        seq66::tick_position(9600));
    CHECK_EQ(end.intention_type, seq66::recording_intention::type::end);

    auto cancel = seq66::recording_intention::make_cancel(1, gen,
        "generation change");
    CHECK_EQ(cancel.intention_type, seq66::recording_intention::type::cancel);
    CHECK_EQ(cancel.reason, std::string("generation change"));
}

/* ------------------------------------------------------------------ */
/*  to_string tests                                                    */
/* ------------------------------------------------------------------ */

static void
test_to_string ()
{
    std::cout << "\n--- to_string ---" << std::endl;
    CHECK_EQ(std::string(seq66::to_string(seq66::recording_state::idle)),
             "idle");
    CHECK_EQ(std::string(seq66::to_string(seq66::recording_state::armed)),
             "armed");
    CHECK_EQ(std::string(seq66::to_string(seq66::recording_state::waiting)),
             "waiting");
    CHECK_EQ(std::string(seq66::to_string(seq66::recording_state::recording)),
             "recording");
    CHECK_EQ(std::string(seq66::to_string(seq66::recording_state::verifying)),
             "verifying");
    CHECK_EQ(std::string(seq66::to_string(seq66::recording_state::complete)),
             "complete");
    CHECK_EQ(std::string(seq66::to_string(seq66::recording_state::failed)),
             "failed");
    CHECK_EQ(std::string(seq66::to_string(seq66::recording_state::indeterminate)),
             "indeterminate");

    CHECK_EQ(std::string(seq66::to_string(seq66::recording_termination::completed_exactly)),
             "completed_exactly");
    CHECK_EQ(std::string(seq66::to_string(seq66::recording_termination::manually_truncated)),
             "manually_truncated");

    CHECK_EQ(std::string(seq66::to_string(seq66::start_boundary::next_bar)),
             "next_bar");
    CHECK_EQ(std::string(seq66::to_string(seq66::start_boundary::next_beat)),
             "next_beat");
    CHECK_EQ(std::string(seq66::to_string(seq66::start_boundary::immediate)),
             "immediate");

    CHECK_EQ(std::string(seq66::to_string(
        seq66::tempo_change_policy::preserve_target_beats)),
        "preserve_target_beats");
}

/* ------------------------------------------------------------------ */
/*  Matrix: time signature × bar count                                 */
/* ------------------------------------------------------------------ */

struct metric_case
{
    const char * name;
    int tpb;        /* ticks per beat */
    int num;        /* numerator */
    int den;        /* denominator */
    int bpb;        /* beats per bar */
    int bars;
    int64_t expected_ticks;
    int64_t expected_beats;
};

static void
test_metric_matrix ()
{
    std::cout << "\n--- Metric matrix ---" << std::endl;

    metric_case cases[] = {
        {"4/4 4 bars",  480, 4, 4, 4, 4,  7680,  16},
        {"4/4 1 bar",   480, 4, 4, 4, 1,  1920,   4},
        {"4/4 8 bars",  480, 4, 4, 4, 8, 15360,  32},
        {"3/4 4 bars",  480, 3, 4, 3, 4,  5760,  12},
        {"3/4 1 bar",   480, 3, 4, 3, 1,  1440,   3},
        {"5/4 4 bars",  480, 5, 4, 5, 4,  9600,  20},
        {"5/4 2 bars",  480, 5, 4, 5, 2,  4800,  10},
        {"6/8 4 bars",  240, 6, 8, 3, 4,  2880,  12},
        {"6/8 2 bars",  240, 6, 8, 3, 2,  1440,   6},
        {"7/8 4 bars",  240, 7, 8, 7, 4,  6720,  28},
        {"7/8 1 bar",   240, 7, 8, 7, 1,  1680,   7},
    };

    for (const auto & tc : cases)
    {
        seq66::musical_metric m;
        m.ticks_per_quarter = tc.tpb;
        m.numerator = tc.num;
        m.denominator = tc.den;
        m.beats_per_bar = tc.bpb;
        m.ticks_per_beat = tc.tpb;

        seq66::tick_count ticks;
        CHECK(m.duration_ticks(seq66::bar_count(tc.bars), ticks));
        CHECK_EQ(ticks.value, tc.expected_ticks);

        seq66::beat_count beats;
        CHECK(m.target_beats(seq66::bar_count(tc.bars), beats));
        CHECK_EQ(beats.value, tc.expected_beats);
    }
}

/* ------------------------------------------------------------------ */
/*  Edge cases                                                         */
/* ------------------------------------------------------------------ */

static void
test_overflow_protection ()
{
    std::cout << "\n--- Overflow protection ---" << std::endl;
    seq66::musical_metric m;
    m.beats_per_bar = 4;
    m.ticks_per_beat = 480;

    /* Large bar count that would overflow */
    seq66::bar_count huge(std::numeric_limits<int64_t>::max());
    seq66::tick_count result;
    CHECK(! m.duration_ticks(huge, result));

    seq66::beat_count beats;
    CHECK(! m.target_beats(huge, beats));
}

static void
test_zero_bars ()
{
    std::cout << "\n--- Zero bars ---" << std::endl;
    seq66::musical_metric m;
    m.beats_per_bar = 4;
    m.ticks_per_beat = 480;

    seq66::tick_count ticks;
    CHECK(! m.duration_ticks(seq66::bar_count(0), ticks));

    seq66::beat_count beats;
    CHECK(! m.target_beats(seq66::bar_count(0), beats));
}

static void
test_negative_values ()
{
    std::cout << "\n--- Negative values ---" << std::endl;
    seq66::tick_position neg(-1);
    CHECK(! neg.is_valid());

    seq66::tick_count neg_count(-1);
    CHECK(! neg_count.is_positive());

    seq66::bar_count neg_bar(-1);
    CHECK(! neg_bar.is_positive());
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int
main ()
{
    test_tick_position();
    test_tick_count();
    test_bar_beat_counts();
    test_transport_generation();
    test_recording_request();
    test_metric_duration_ticks();
    test_metric_target_beats();
    test_metric_invalid();
    test_recording_plan();
    test_recording_intention();
    test_to_string();
    test_metric_matrix();
    test_overflow_protection();
    test_zero_bars();
    test_negative_values();

    std::cout << "\n" << g_total << "/" << g_total
              << " pass. All recording types assertions passed." << std::endl;
    return g_failed;
}
