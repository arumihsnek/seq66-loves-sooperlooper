/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_transport_tempo_policy_test.cpp
 *
 *  Tests for transport and tempo policies.
 *
 *  Covers:
 *  - Transport: sync source mapping, start/stop transitions,
 *    sync mode queries, string conversion, parse.
 *  - Tempo: mode switching (free/tape/elastic), rate computation,
 *    add/remove loops, recompute, global tempo, edge cases.
 */

#include "audio/sooperlooper_transport_policy.hpp"
#include "audio/sooperlooper_tempo_policy.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

static int s_assertions = 0;

#define CHECK(expr) \
    do { ++s_assertions; if (! (expr)) { \
        std::fprintf(stderr, "  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } } while (0)

#define CHECK_EQ(a, b) \
    do { ++s_assertions; if ((a) != (b)) { \
        std::fprintf(stderr, "  [FAIL] %s:%d: %s == %s\n", \
            __FILE__, __LINE__, #a, #b); \
        return 1; \
    } } while (0)

#define CHECK_NEAR(a, b, eps) \
    do { ++s_assertions; if (std::fabs((a) - (b)) > (eps)) { \
        std::fprintf(stderr, "  [FAIL] %s:%d: |%s - %s| > %f\n", \
            __FILE__, __LINE__, #a, #b, (double)(eps)); \
        return 1; \
    } } while (0)

/* ------------------------------------------------------------------ */
/*  Transport policy tests                                            */
/* ------------------------------------------------------------------ */

static int
test_transport_initial_state ()
{
    seq66::sooperlooper_transport_policy tp;

    CHECK_EQ(static_cast<int>(tp.sync_source_value()), -1);
    CHECK_EQ(tp.state(), seq66::transport_state::stopped);
    CHECK(tp.is_jack_sync());
    CHECK(! tp.is_midi_sync());
    CHECK(! tp.is_no_sync());
    return 0;
}

static int
test_transport_sync_source_mapping ()
{
    seq66::sooperlooper_transport_policy tp;

    /* JACK = -1 */
    tp.set_sync_source("jack");
    CHECK(tp.is_jack_sync());
    CHECK_EQ(tp.sync_source_value(), -1);

    /* MIDI = -2 */
    tp.set_sync_source("midi");
    CHECK(tp.is_midi_sync());
    CHECK_EQ(tp.sync_source_value(), -2);

    /* Internal = -3 */
    tp.set_sync_source("internal");
    CHECK_EQ(tp.sync_source_value(), -3);

    /* None = 0 */
    tp.set_sync_source("none");
    CHECK(tp.is_no_sync());
    CHECK_EQ(tp.sync_source_value(), 0);

    /* Unknown string rejected */
    CHECK(! tp.set_sync_source("banana"));
    CHECK(tp.is_no_sync());

    return 0;
}

static int
test_transport_sync_from_value ()
{
    seq66::sooperlooper_transport_policy tp;

    tp.set_sync_source_from_value(-3);
    CHECK_EQ(tp.sync_source_value(), -3);

    tp.set_sync_source_from_value(-2);
    CHECK_EQ(tp.sync_source_value(), -2);

    tp.set_sync_source_from_value(-1);
    CHECK_EQ(tp.sync_source_value(), -1);

    tp.set_sync_source_from_value(0);
    CHECK_EQ(tp.sync_source_value(), 0);

    tp.set_sync_source_from_value(42);
    CHECK_EQ(tp.sync_source_value(), 0);

    return 0;
}

static int
test_transport_start_stop ()
{
    seq66::sooperlooper_transport_policy tp;

    CHECK_EQ(tp.state(), seq66::transport_state::stopped);

    /* Start from stopped */
    CHECK(tp.request_start());
    CHECK_EQ(tp.state(), seq66::transport_state::playing);

    /* Stop from playing */
    CHECK(tp.request_stop());
    CHECK_EQ(tp.state(), seq66::transport_state::stopped);

    /* Start from playing = no-op */
    CHECK(tp.request_start());
    CHECK(! tp.request_start());

    /* Stop from stopped = no-op */
    CHECK(tp.request_stop());
    CHECK(! tp.request_stop());

    return 0;
}

static int
test_transport_config ()
{
    seq66::sooperlooper_transport_policy tp;
    seq66::transport_config cfg;
    cfg.sync_source = seq66::transport_sync_source::midi;
    cfg.use_midi_start = true;
    cfg.output_midi_clock = true;

    tp.set_config(cfg);
    CHECK(tp.config().sync_source == seq66::transport_sync_source::midi);
    CHECK(tp.config().use_midi_start);
    CHECK(tp.config().output_midi_clock);
    CHECK_EQ(tp.sync_source_value(), -2);

    return 0;
}

static int
test_transport_string_conversion ()
{
    CHECK_EQ(std::strcmp(
        seq66::to_string(seq66::transport_sync_source::none), "none"), 0);
    CHECK_EQ(std::strcmp(
        seq66::to_string(seq66::transport_sync_source::jack), "jack"), 0);
    CHECK_EQ(std::strcmp(
        seq66::to_string(seq66::transport_sync_source::midi), "midi"), 0);
    CHECK_EQ(std::strcmp(
        seq66::to_string(seq66::transport_sync_source::internal),
        "internal"), 0);

    CHECK_EQ(std::strcmp(
        seq66::to_string(seq66::transport_state::stopped), "stopped"), 0);
    CHECK_EQ(std::strcmp(
        seq66::to_string(seq66::transport_state::playing), "playing"), 0);

    return 0;
}

static int
test_transport_parse ()
{
    seq66::transport_sync_source src;

    CHECK(seq66::try_parse("none", src));
    CHECK(src == seq66::transport_sync_source::none);

    CHECK(seq66::try_parse("jack", src));
    CHECK(src == seq66::transport_sync_source::jack);

    CHECK(seq66::try_parse("midi", src));
    CHECK(src == seq66::transport_sync_source::midi);

    CHECK(seq66::try_parse("internal", src));
    CHECK(src == seq66::transport_sync_source::internal);

    CHECK(! seq66::try_parse("invalid", src));
    CHECK(! seq66::try_parse("", src));

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Tempo policy tests                                                */
/* ------------------------------------------------------------------ */

static int
test_tempo_initial_state ()
{
    seq66::sooperlooper_tempo_policy tp;

    CHECK_EQ(tp.mode(), seq66::tempo_mode::free);
    CHECK_NEAR(tp.global_config().tempo, 120.0f, 0.01f);
    CHECK_EQ(static_cast<int>(tp.loop_configs().size()), 0);
    return 0;
}

static int
test_tempo_free_mode ()
{
    seq66::sooperlooper_tempo_policy tp;
    tp.set_global_tempo(140.0f);
    int idx = tp.add_loop(120.0f);

    tp.set_mode(seq66::tempo_mode::free);

    const auto * lc = tp.loop_config(idx);
    CHECK(lc != nullptr);
    CHECK(! lc->use_rate);
    CHECK(! lc->tempo_stretch);
    CHECK_NEAR(lc->rate, 1.0f, 0.01f);
    CHECK(! lc->round_integer_tempo);
    return 0;
}

static int
test_tempo_tape_mode ()
{
    seq66::sooperlooper_tempo_policy tp;
    tp.set_global_tempo(140.0f);
    int idx = tp.add_loop(120.0f);

    tp.set_mode(seq66::tempo_mode::tape);

    const auto * lc = tp.loop_config(idx);
    CHECK(lc != nullptr);
    CHECK(lc->use_rate);
    CHECK(! lc->tempo_stretch);
    CHECK_NEAR(lc->rate, 1.0f, 0.01f);
    CHECK(! lc->round_integer_tempo);
    return 0;
}

static int
test_tempo_elastic_mode ()
{
    seq66::sooperlooper_tempo_policy tp;
    tp.set_global_tempo(120.0f);
    int idx = tp.add_loop(120.0f);

    tp.set_mode(seq66::tempo_mode::elastic);

    const auto * lc = tp.loop_config(idx);
    CHECK(lc != nullptr);
    CHECK(lc->use_rate);
    CHECK(lc->tempo_stretch);
    CHECK_NEAR(lc->rate, 1.0f, 0.01f);
    CHECK(lc->round_integer_tempo);
    return 0;
}

static int
test_tempo_elastic_rate_computation ()
{
    seq66::sooperlooper_tempo_policy tp;

    /* 120 BPM loop at 60 BPM global -> rate 0.5 */
    CHECK_NEAR(tp.compute_rate(60.0f, 120.0f), 0.5f, 0.01f);

    /* 120 BPM loop at 120 BPM global -> rate 1.0 */
    CHECK_NEAR(tp.compute_rate(120.0f, 120.0f), 1.0f, 0.01f);

    /* 120 BPM loop at 240 BPM global -> rate 2.0 */
    CHECK_NEAR(tp.compute_rate(240.0f, 120.0f), 2.0f, 0.01f);

    /* 120 BPM loop at 480 BPM global -> rate clamped 4.0 */
    CHECK_NEAR(tp.compute_rate(480.0f, 120.0f), 4.0f, 0.01f);

    /* 120 BPM loop at 30 BPM global -> rate clamped 0.25 */
    CHECK_NEAR(tp.compute_rate(30.0f, 120.0f), 0.25f, 0.01f);

    /* Zero recording tempo -> rate 1.0 (safety) */
    CHECK_NEAR(tp.compute_rate(120.0f, 0.0f), 1.0f, 0.01f);

    return 0;
}

static int
test_tempo_global_tempo_change ()
{
    seq66::sooperlooper_tempo_policy tp;
    tp.set_global_tempo(120.0f);
    int idx = tp.add_loop(120.0f);
    tp.set_mode(seq66::tempo_mode::elastic);

    /* Rate 1.0 at 120 BPM */
    CHECK_NEAR(tp.loop_config(idx)->rate, 1.0f, 0.01f);

    /* Change global tempo to 150 BPM */
    tp.set_global_tempo(150.0f);
    CHECK_NEAR(tp.global_config().tempo, 150.0f, 0.01f);
    CHECK_NEAR(tp.loop_config(idx)->rate, 1.25f, 0.01f);

    /* Change to 60 BPM */
    tp.set_global_tempo(60.0f);
    CHECK_NEAR(tp.loop_config(idx)->rate, 0.5f, 0.01f);

    return 0;
}

static int
test_tempo_add_remove_loops ()
{
    seq66::sooperlooper_tempo_policy tp;
    tp.set_global_tempo(120.0f);
    tp.set_mode(seq66::tempo_mode::elastic);

    int i0 = tp.add_loop(120.0f);
    int i1 = tp.add_loop(60.0f);
    int i2 = tp.add_loop(240.0f);

    CHECK_EQ(i0, 0);
    CHECK_EQ(i1, 1);
    CHECK_EQ(i2, 2);
    CHECK_EQ(static_cast<int>(tp.loop_configs().size()), 3);

    CHECK_NEAR(tp.loop_config(i0)->rate, 1.0f, 0.01f);
    CHECK_NEAR(tp.loop_config(i1)->rate, 2.0f, 0.01f);
    CHECK_NEAR(tp.loop_config(i2)->rate, 0.5f, 0.01f);

    /* Remove middle */
    CHECK(tp.remove_loop(1));
    CHECK_EQ(static_cast<int>(tp.loop_configs().size()), 2);

    /* Out of range */
    CHECK(! tp.remove_loop(-1));
    CHECK(! tp.remove_loop(5));

    return 0;
}

static int
test_tempo_clear_loops ()
{
    seq66::sooperlooper_tempo_policy tp;
    tp.add_loop(120.0f);
    tp.add_loop(140.0f);
    CHECK_EQ(static_cast<int>(tp.loop_configs().size()), 2);

    tp.clear_loops();
    CHECK_EQ(static_cast<int>(tp.loop_configs().size()), 0);
    CHECK(tp.loop_config(0) == nullptr);

    return 0;
}

static int
test_tempo_mode_string ()
{
    CHECK_EQ(std::strcmp(
        seq66::to_string(seq66::tempo_mode::free), "free"), 0);
    CHECK_EQ(std::strcmp(
        seq66::to_string(seq66::tempo_mode::tape), "tape"), 0);
    CHECK_EQ(std::strcmp(
        seq66::to_string(seq66::tempo_mode::elastic), "elastic"), 0);
    return 0;
}

static int
test_tempo_parse ()
{
    seq66::tempo_mode m;

    CHECK(seq66::try_parse("free", m));
    CHECK(m == seq66::tempo_mode::free);

    CHECK(seq66::try_parse("tape", m));
    CHECK(m == seq66::tempo_mode::tape);

    CHECK(seq66::try_parse("elastic", m));
    CHECK(m == seq66::tempo_mode::elastic);

    CHECK(! seq66::try_parse("invalid", m));
    CHECK(! seq66::try_parse("", m));

    return 0;
}

static int
test_tempo_set_mode_string ()
{
    seq66::sooperlooper_tempo_policy tp;
    int idx = tp.add_loop(120.0f);

    CHECK(tp.set_mode("elastic"));
    CHECK_EQ(tp.mode(), seq66::tempo_mode::elastic);
    CHECK(tp.loop_config(idx)->use_rate);

    CHECK(tp.set_mode("tape"));
    CHECK_EQ(tp.mode(), seq66::tempo_mode::tape);

    CHECK(! tp.set_mode("banana"));
    CHECK_EQ(tp.mode(), seq66::tempo_mode::tape);

    return 0;
}

static int
test_tempo_global_tempo_clamp ()
{
    seq66::sooperlooper_tempo_policy tp;

    tp.set_global_tempo(-10.0f);
    CHECK_NEAR(tp.global_config().tempo, 0.0f, 0.01f);

    tp.set_global_tempo(2000.0f);
    CHECK_NEAR(tp.global_config().tempo, 1000.0f, 0.01f);

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */

int
main ()
{
    std::printf("=== Transport and tempo policy tests ===\n\n");

    struct test { const char * name; int (*fn)(); };
    struct test tests[] = {
        {"transport: initial state",           test_transport_initial_state},
        {"transport: sync source mapping",     test_transport_sync_source_mapping},
        {"transport: sync from value",         test_transport_sync_from_value},
        {"transport: start/stop",              test_transport_start_stop},
        {"transport: config",                  test_transport_config},
        {"transport: string conversion",       test_transport_string_conversion},
        {"transport: parse",                   test_transport_parse},
        {"tempo: initial state",               test_tempo_initial_state},
        {"tempo: free mode",                   test_tempo_free_mode},
        {"tempo: tape mode",                   test_tempo_tape_mode},
        {"tempo: elastic mode",                test_tempo_elastic_mode},
        {"tempo: elastic rate computation",    test_tempo_elastic_rate_computation},
        {"tempo: global tempo change",         test_tempo_global_tempo_change},
        {"tempo: add/remove loops",            test_tempo_add_remove_loops},
        {"tempo: clear loops",                 test_tempo_clear_loops},
        {"tempo: mode string",                 test_tempo_mode_string},
        {"tempo: parse",                       test_tempo_parse},
        {"tempo: set mode string",             test_tempo_set_mode_string},
        {"tempo: global tempo clamp",          test_tempo_global_tempo_clamp},
    };

    int count = sizeof(tests) / sizeof(tests[0]);
    int failures = 0;

    for (int i = 0; i < count; ++i)
    {
        int rc = tests[i].fn();
        const char * mark = rc == 0 ? "PASS" : "FAIL";
        std::printf("  [%s] %s\n", mark, tests[i].name);
        if (rc != 0)
            ++failures;
    }

    std::printf("\n%d assertions, %d failures.\n", s_assertions, failures);
    return failures;
}
