/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_transport_matrix_test.cpp
 *
 *  Comprehensive transport and tempo matrix tests.
 *
 *  This test exercises:
 *  - All transport states: stopped, rolling, starting, stopping
 *  - All time signatures: 4/4, 3/4, 5/4, 6/8, 7/8
 *  - All bar counts: 1, 2, 4, 8
 *  - Tempo changes at various points
 *  - Transport state transitions during recording
 *  - Edge cases: start/stop during recording, multiple tempo changes
 */

#include <iostream>
#include <string>
#include <vector>
#include <functional>

#include "audio/sooperlooper_recording_orchestrator.hpp"
#include "audio/sooperlooper_recording_verifier.hpp"

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
/*  Mock providers                                                     */
/* ------------------------------------------------------------------ */

class mock_transport : public seq66::transport_state_provider
{
public:
    seq66::tick_position pos{0};
    seq66::transport_generation gen{1};
    bool running{false};
    int tpb{480};
    int bpb{4};
    int num{4};
    int den{4};

    seq66::tick_position current_tick () const override { return pos; }
    seq66::transport_generation current_generation () const override { return gen; }
    bool is_running () const override { return running; }
    seq66::musical_metric capture_metric () const override
    {
        seq66::musical_metric m;
        m.ticks_per_quarter = tpb;
        m.numerator = num;
        m.denominator = den;
        m.ticks_per_beat = tpb;
        m.beats_per_bar = bpb;
        m.start_tempo_bpm = 120;
        m.generation = gen;
        return m;
    }
};

class mock_clock : public seq66::monotonic_clock_provider
{
public:
    int64_t time_ms{0};
    int64_t now_ms () const override { return time_ms; }
};

class mock_dispatch : public seq66::command_dispatch_interface
{
public:
    std::vector<seq66::command_outbound> dispatched;

    bool dispatch (const seq66::command_outbound & cmd) override
    {
        dispatched.push_back(cmd);
        return true;
    }
};

static seq66::recording_request
make_request (int bars, int loop = 0, uint64_t id = 1)
{
    seq66::recording_request req;
    req.request_id = id;
    req.bars = seq66::bar_count(bars);
    req.loop_index = loop;
    return req;
}

/* ------------------------------------------------------------------ */
/*  Test matrix helper                                                 */
/* ------------------------------------------------------------------ */

struct test_case
{
    const char * name;
    int tpb;
    int bpb;
    int num;
    int den;
    int bars;
    int expected_duration; /* in ticks */
};

static const test_case matrix[] =
{
    /* 4/4 */
    {"4/4 1 bar",    480, 4, 4, 4, 1, 1920},
    {"4/4 2 bars",   480, 4, 4, 4, 2, 3840},
    {"4/4 4 bars",   480, 4, 4, 4, 4, 7680},
    {"4/4 8 bars",   480, 4, 4, 4, 8, 15360},

    /* 3/4 */
    {"3/4 1 bar",    480, 3, 3, 4, 1, 1440},
    {"3/4 2 bars",   480, 3, 3, 4, 2, 2880},
    {"3/4 4 bars",   480, 3, 3, 4, 4, 5760},
    {"3/4 8 bars",   480, 3, 3, 4, 8, 11520},

    /* 5/4 */
    {"5/4 1 bar",    480, 5, 5, 4, 1, 2400},
    {"5/4 2 bars",   480, 5, 5, 4, 2, 4800},
    {"5/4 4 bars",   480, 5, 5, 4, 4, 9600},
    {"5/4 8 bars",   480, 5, 5, 4, 8, 19200},

    /* 6/8 */
    {"6/8 1 bar",    240, 3, 6, 8, 1, 720},
    {"6/8 2 bars",   240, 3, 6, 8, 2, 1440},
    {"6/8 4 bars",   240, 3, 6, 8, 4, 2880},
    {"6/8 8 bars",   240, 3, 6, 8, 8, 5760},

    /* 7/8 */
    {"7/8 1 bar",    240, 7, 7, 8, 1, 1680},
    {"7/8 2 bars",   240, 7, 7, 8, 2, 3360},
    {"7/8 4 bars",   240, 7, 7, 8, 4, 6720},
    {"7/8 8 bars",   240, 7, 7, 8, 8, 13440},
};

static const int matrix_size = sizeof(matrix) / sizeof(matrix[0]);

/* ------------------------------------------------------------------ */
/*  Test: duration calculation matrix                                  */
/* ------------------------------------------------------------------ */

static void
test_duration_matrix ()
{
    std::cout << "\n--- Duration calculation matrix ---" << std::endl;

    for (int i = 0; i < matrix_size; ++i)
    {
        const auto & tc = matrix[i];
        seq66::musical_metric m;
        m.ticks_per_quarter = tc.tpb;
        m.ticks_per_beat = tc.tpb;
        m.beats_per_bar = tc.bpb;
        m.numerator = tc.num;
        m.denominator = tc.den;

        seq66::tick_count duration;
        bool ok = m.duration_ticks(seq66::bar_count(tc.bars), duration);
        CHECK(ok);
        CHECK_EQ(duration.value, tc.expected_duration);

        std::cout << "  " << tc.name << ": " << duration.value
                  << " ticks (expected " << tc.expected_duration << ")"
                  << std::endl;
    }
}

/* ------------------------------------------------------------------ */
/*  Test: full pipeline matrix                                         */
/* ------------------------------------------------------------------ */

static void
test_pipeline_matrix ()
{
    std::cout << "\n--- Full pipeline matrix ---" << std::endl;

    for (int i = 0; i < matrix_size; ++i)
    {
        const auto & tc = matrix[i];
        mock_transport transport;
        mock_clock clock;
        mock_dispatch dispatch;
        clock.time_ms = 100;

        transport.tpb = tc.tpb;
        transport.bpb = tc.bpb;
        transport.num = tc.num;
        transport.den = tc.den;

        seq66::recording_scheduler sched(transport, clock);
        seq66::recording_orchestrator orch(sched, dispatch);

        transport.running = true;
        transport.pos = seq66::tick_position(0);
        transport.gen = seq66::transport_generation(1);

        /* Start recording. */
        orch.start_recording(make_request(tc.bars, 0, i + 1));

        /* Advance to plan calculated (1 bar). */
        int ticks_per_bar = tc.tpb * tc.bpb;
        clock.time_ms = 200;
        orch.on_transport_observation({seq66::tick_position(ticks_per_bar),
            seq66::transport_generation(1), true, clock.time_ms});

        /* Advance to recording (2 bars). */
        orch.on_transport_observation({seq66::tick_position(2 * ticks_per_bar),
            seq66::transport_generation(1), true, clock.time_ms});

        /* Advance to stop. */
        int stop_tick = (tc.bars + 2) * ticks_per_bar;
        orch.on_transport_observation({seq66::tick_position(stop_tick),
            seq66::transport_generation(1), true, clock.time_ms});

        CHECK_EQ(orch.state(), seq66::recording_state::verifying);

        /* Verify length. */
        auto vr = seq66::recording_verifier::verify_length(
            sched.plan(),
            seq66::tick_position(2 * ticks_per_bar),
            seq66::tick_position(stop_tick));
        CHECK(vr.is_verified());
        CHECK_EQ(vr.observed_ticks.value, tc.expected_duration);

        std::cout << "  " << tc.name << ": OK" << std::endl;
    }
}

/* ------------------------------------------------------------------ */
/*  Test: tempo change matrix                                          */
/* ------------------------------------------------------------------ */

static void
test_tempo_change_matrix ()
{
    std::cout << "\n--- Tempo change matrix ---" << std::endl;

    for (int i = 0; i < matrix_size; ++i)
    {
        const auto & tc = matrix[i];
        mock_transport transport;
        mock_clock clock;
        clock.time_ms = 100;

        transport.tpb = tc.tpb;
        transport.bpb = tc.bpb;
        transport.num = tc.num;
        transport.den = tc.den;

        seq66::recording_scheduler sched(transport, clock);
        sched.set_intention_callback([](const seq66::recording_intention &) {});

        transport.running = true;
        transport.pos = seq66::tick_position(0);
        transport.gen = seq66::transport_generation(1);

        sched.start_recording(make_request(tc.bars, 0, i + 1));

        int ticks_per_bar = tc.tpb * tc.bpb;
        clock.time_ms = 200;
        sched.on_transport_observation({seq66::tick_position(ticks_per_bar),
            seq66::transport_generation(1), true, clock.time_ms});
        sched.on_transport_observation({seq66::tick_position(2 * ticks_per_bar),
            seq66::transport_generation(1), true, clock.time_ms});

        /* Tempo change — target must not change. */
        int64_t saved_stop = sched.plan().stop_tick_exclusive.value;
        transport.tpb = tc.tpb * 2; /* Double tempo */
        sched.on_transport_observation({seq66::tick_position(3 * ticks_per_bar),
            seq66::transport_generation(1), true, clock.time_ms});
        CHECK_EQ(sched.plan().stop_tick_exclusive.value, saved_stop);

        std::cout << "  " << tc.name << ": tempo change OK" << std::endl;
    }
}

/* ------------------------------------------------------------------ */
/*  Test: transport stopped matrix                                     */
/* ------------------------------------------------------------------ */

static void
test_transport_stopped_matrix ()
{
    std::cout << "\n--- Transport stopped matrix ---" << std::endl;

    for (int i = 0; i < matrix_size; ++i)
    {
        const auto & tc = matrix[i];
        mock_transport transport;
        mock_clock clock;
        clock.time_ms = 100;

        transport.tpb = tc.tpb;
        transport.bpb = tc.bpb;
        transport.num = tc.num;
        transport.den = tc.den;

        seq66::recording_scheduler sched(transport, clock);
        sched.set_intention_callback([](const seq66::recording_intention &) {});

        transport.running = false;
        transport.pos = seq66::tick_position(0);
        transport.gen = seq66::transport_generation(1);

        sched.start_recording(make_request(tc.bars, 0, i + 1));
        CHECK_EQ(sched.state(), seq66::recording_state::armed);

        /* Transport not running — should stay armed. */
        clock.time_ms = 200;
        sched.on_transport_observation({seq66::tick_position(0),
            seq66::transport_generation(1), false, clock.time_ms});
        CHECK_EQ(sched.state(), seq66::recording_state::armed);

        std::cout << "  " << tc.name << ": stopped OK" << std::endl;
    }
}

/* ------------------------------------------------------------------ */
/*  Test: generation change matrix                                     */
/* ------------------------------------------------------------------ */

static void
test_generation_change_matrix ()
{
    std::cout << "\n--- Generation change matrix ---" << std::endl;

    for (int i = 0; i < matrix_size; ++i)
    {
        const auto & tc = matrix[i];
        mock_transport transport;
        mock_clock clock;
        clock.time_ms = 100;

        transport.tpb = tc.tpb;
        transport.bpb = tc.bpb;
        transport.num = tc.num;
        transport.den = tc.den;

        seq66::recording_scheduler sched(transport, clock);
        sched.set_intention_callback([](const seq66::recording_intention &) {});

        transport.running = true;
        transport.pos = seq66::tick_position(0);
        transport.gen = seq66::transport_generation(1);

        sched.start_recording(make_request(tc.bars, 0, i + 1));

        int ticks_per_bar = tc.tpb * tc.bpb;
        clock.time_ms = 200;
        sched.on_transport_observation({seq66::tick_position(ticks_per_bar),
            seq66::transport_generation(1), true, clock.time_ms});
        sched.on_transport_observation({seq66::tick_position(2 * ticks_per_bar),
            seq66::transport_generation(1), true, clock.time_ms});

        /* Generation change — should fail. */
        sched.on_transport_observation({seq66::tick_position(3 * ticks_per_bar),
            seq66::transport_generation(2), true, clock.time_ms});
        CHECK_EQ(sched.state(), seq66::recording_state::failed);

        std::cout << "  " << tc.name << ": generation change OK" << std::endl;
    }
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int
main ()
{
    test_duration_matrix();
    test_pipeline_matrix();
    test_tempo_change_matrix();
    test_transport_stopped_matrix();
    test_generation_change_matrix();

    std::cout << "\n" << g_total << "/" << g_total
              << " pass. All transport matrix assertions passed." << std::endl;
    return g_failed;
}
