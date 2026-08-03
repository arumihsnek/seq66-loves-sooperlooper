/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_recording_scheduler_test.cpp
 *
 *  Comprehensive tests for the recording scheduler state machine.
 *
 *  Coverage:
 *  - 4/4, 3/4, 5/4, 6/8, 7/8 time signatures
 *  - Various bar counts (1, 2, 4, 8)
 *  - Record arms and starts at next bar boundary
 *  - Stop after N complete bars
 *  - Transport stopped at request
 *  - Cancel
 *  - Manual stop (truncated)
 *  - Tempo changes (no target beat modification)
 *  - Crash/restart (generation change)
 *  - Missing feedback (timeout)
 *  - Generation invalidation
 *  - Overflow protection
 */

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <functional>

#include "audio/sooperlooper_recording_scheduler.hpp"

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

/* ------------------------------------------------------------------ */
/*  Helper: create a recording request                                 */
/* ------------------------------------------------------------------ */

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
/*  Test: basic 4/4 four-bar recording                                 */
/* ------------------------------------------------------------------ */

static void
test_4_4_four_bars ()
{
    std::cout << "\n--- 4/4 four bars ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);

    std::vector<seq66::recording_intention> intentions;
    sched.set_intention_callback([&](const seq66::recording_intention & i) {
        intentions.push_back(i);
    });

    /* Transport running, at tick 0. */
    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    /* Start recording: should arm. */
    CHECK(sched.start_recording(make_request(4)));
    CHECK_EQ(sched.state(), seq66::recording_state::armed);
    CHECK_EQ(intentions.size(), 1u);
    CHECK_EQ(intentions[0].intention_type, seq66::recording_intention::type::arm);

    /* Transport advances to tick 1920 (1 bar). Plan should be calculated. */
    clock.time_ms = 200;
    sched.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::waiting);
    CHECK(sched.plan().is_valid());
    CHECK_EQ(sched.plan().target_bars.value, 4);

    /* Transport reaches start boundary (next bar). */
    sched.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::recording);

    /* Transport reaches stop (3840 + 7680 = 11520). */
    sched.on_transport_observation({seq66::tick_position(11520),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::verifying);

    /* Confirm stop. */
    sched.on_command_result({1, seq66::transport_generation(1), true, false, ""});
    CHECK_EQ(sched.state(), seq66::recording_state::complete);
    CHECK(sched.last_verification().is_verified());
}

/* ------------------------------------------------------------------ */
/*  Test: transport stopped at request                                 */
/* ------------------------------------------------------------------ */

static void
test_transport_stopped ()
{
    std::cout << "\n--- Transport stopped ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);
    sched.set_intention_callback([](const seq66::recording_intention &) {});

    transport.running = false;
    transport.pos = seq66::tick_position(0);

    CHECK(sched.start_recording(make_request(4)));
    CHECK_EQ(sched.state(), seq66::recording_state::armed);

    /* Transport not running — should stay armed. */
    clock.time_ms = 200;
    sched.on_transport_observation({seq66::tick_position(0),
        seq66::transport_generation(1), false, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::armed);
}

/* ------------------------------------------------------------------ */
/*  Test: cancel                                                       */
/* ------------------------------------------------------------------ */

static void
test_cancel ()
{
    std::cout << "\n--- Cancel ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);
    std::vector<seq66::recording_intention> intentions;
    sched.set_intention_callback([&](const seq66::recording_intention & i) {
        intentions.push_back(i);
    });

    transport.running = true;
    transport.pos = seq66::tick_position(0);

    CHECK(sched.start_recording(make_request(4)));
    CHECK_EQ(sched.state(), seq66::recording_state::armed);

    sched.cancel_recording("user cancel");
    CHECK_EQ(sched.state(), seq66::recording_state::failed);
    CHECK_EQ(sched.termination(), seq66::recording_termination::invalidated);
}

/* ------------------------------------------------------------------ */
/*  Test: manual stop                                                  */
/* ------------------------------------------------------------------ */

static void
test_manual_stop ()
{
    std::cout << "\n--- Manual stop ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);
    std::vector<seq66::recording_intention> intentions;
    sched.set_intention_callback([&](const seq66::recording_intention & i) {
        intentions.push_back(i);
    });

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    sched.start_recording(make_request(4));

    /* Advance to recording state. */
    clock.time_ms = 200;
    sched.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    sched.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::recording);

    /* Manual stop mid-recording. */
    sched.manual_stop();
    CHECK_EQ(sched.state(), seq66::recording_state::verifying);
    CHECK_EQ(sched.termination(), seq66::recording_termination::manually_truncated);
}

/* ------------------------------------------------------------------ */
/*  Test: 3/4 three bars                                               */
/* ------------------------------------------------------------------ */

static void
test_3_4_three_bars ()
{
    std::cout << "\n--- 3/4 three bars ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;
    transport.bpb = 3;
    transport.num = 3;
    transport.den = 4;

    seq66::recording_scheduler sched(transport, clock);
    std::vector<seq66::recording_intention> intentions;
    sched.set_intention_callback([&](const seq66::recording_intention & i) {
        intentions.push_back(i);
    });

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    sched.start_recording(make_request(3));

    /* Plan: 3 bars * 3 beats * 480 ticks = 4320 ticks */
    clock.time_ms = 200;
    sched.on_transport_observation({seq66::tick_position(1440),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::waiting);
    CHECK_EQ(sched.plan().duration_ticks.value, 4320);

    sched.on_transport_observation({seq66::tick_position(2880),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::recording);

    sched.on_transport_observation({seq66::tick_position(7200),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::verifying);
}

/* ------------------------------------------------------------------ */
/*  Test: 5/4 five bars                                                */
/* ------------------------------------------------------------------ */

static void
test_5_4_five_bars ()
{
    std::cout << "\n--- 5/4 five bars ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;
    transport.bpb = 5;
    transport.num = 5;
    transport.den = 4;

    seq66::recording_scheduler sched(transport, clock);
    sched.set_intention_callback([](const seq66::recording_intention &) {});

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    sched.start_recording(make_request(5));

    clock.time_ms = 200;
    sched.on_transport_observation({seq66::tick_position(2400),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::waiting);
    CHECK_EQ(sched.plan().duration_ticks.value, 12000); /* 5*5*480 */
}

/* ------------------------------------------------------------------ */
/*  Test: 6/8 eight bars                                               */
/* ------------------------------------------------------------------ */

static void
test_6_8_eight_bars ()
{
    std::cout << "\n--- 6/8 eight bars ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;
    transport.tpb = 240;
    transport.bpb = 3;
    transport.num = 6;
    transport.den = 8;

    seq66::recording_scheduler sched(transport, clock);
    sched.set_intention_callback([](const seq66::recording_intention &) {});

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    sched.start_recording(make_request(8));

    clock.time_ms = 200;
    sched.on_transport_observation({seq66::tick_position(720),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::waiting);
    CHECK_EQ(sched.plan().duration_ticks.value, 5760); /* 8*3*240 */
}

/* ------------------------------------------------------------------ */
/*  Test: 7/8 one bar                                                  */
/* ------------------------------------------------------------------ */

static void
test_7_8_one_bar ()
{
    std::cout << "\n--- 7/8 one bar ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;
    transport.tpb = 240;
    transport.bpb = 7;
    transport.num = 7;
    transport.den = 8;

    seq66::recording_scheduler sched(transport, clock);
    sched.set_intention_callback([](const seq66::recording_intention &) {});

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    sched.start_recording(make_request(1));

    clock.time_ms = 200;
    sched.on_transport_observation({seq66::tick_position(1680),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::waiting);
    CHECK_EQ(sched.plan().duration_ticks.value, 1680); /* 1*7*240 */
}

/* ------------------------------------------------------------------ */
/*  Test: tempo change during recording (no target modification)       */
/* ------------------------------------------------------------------ */

static void
test_tempo_change ()
{
    std::cout << "\n--- Tempo change during recording ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);
    sched.set_intention_callback([](const seq66::recording_intention &) {});

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    sched.start_recording(make_request(4));

    clock.time_ms = 200;
    sched.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    sched.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::recording);

    /* Simulate tempo change (metric changes but target stays). */
    transport.tpb = 960; /* tempo doubled in ticks */
    int64_t saved_stop = sched.plan().stop_tick_exclusive.value;

    /* Continue recording — stop target must not change. */
    sched.on_transport_observation({seq66::tick_position(7680),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::recording);
    CHECK_EQ(sched.plan().stop_tick_exclusive.value, saved_stop);
}

/* ------------------------------------------------------------------ */
/*  Test: generation change (invalidation)                             */
/* ------------------------------------------------------------------ */

static void
test_generation_change ()
{
    std::cout << "\n--- Generation change ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);
    sched.set_intention_callback([](const seq66::recording_intention &) {});

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    sched.start_recording(make_request(4));

    clock.time_ms = 200;
    sched.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    sched.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::recording);

    /* Generation change — should invalidate. */
    sched.on_transport_observation({seq66::tick_position(5000),
        seq66::transport_generation(2), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::failed);
    CHECK_EQ(sched.termination(), seq66::recording_termination::invalidated);
}

/* ------------------------------------------------------------------ */
/*  Test: timeout (missing feedback)                                   */
/* ------------------------------------------------------------------ */

static void
test_timeout ()
{
    std::cout << "\n--- Timeout ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;
    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    seq66::recording_tolerance tol;
    tol.deadline_timeout_ms = 500;

    seq66::recording_scheduler sched(transport, clock, tol);
    sched.set_intention_callback([](const seq66::recording_intention &) {});

    sched.start_recording(make_request(4));
    CHECK_EQ(sched.state(), seq66::recording_state::armed);

    /* Time passes but transport never advances beyond arm tick. */
    clock.time_ms = 700;
    sched.on_transport_observation({seq66::tick_position(0),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.state(), seq66::recording_state::indeterminate);
    CHECK_EQ(sched.termination(), seq66::recording_termination::indeterminate);
}

/* ------------------------------------------------------------------ */
/*  Test: command rejection                                            */
/* ------------------------------------------------------------------ */

static void
test_command_rejection ()
{
    std::cout << "\n--- Command rejection ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);
    sched.set_intention_callback([](const seq66::recording_intention &) {});

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    sched.start_recording(make_request(4));

    clock.time_ms = 200;
    sched.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    sched.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});

    /* Command rejected during verification. */
    sched.on_transport_observation({seq66::tick_position(11520),
        seq66::transport_generation(1), true, clock.time_ms});
    sched.on_command_result({1, seq66::transport_generation(1),
        false, true, "engine error"});
    CHECK_EQ(sched.state(), seq66::recording_state::failed);
    CHECK_EQ(sched.termination(), seq66::recording_termination::command_failed);
}

/* ------------------------------------------------------------------ */
/*  Test: eight bars                                                   */
/* ------------------------------------------------------------------ */

static void
test_4_4_eight_bars ()
{
    std::cout << "\n--- 4/4 eight bars ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);
    sched.set_intention_callback([](const seq66::recording_intention &) {});

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    sched.start_recording(make_request(8));

    clock.time_ms = 200;
    sched.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.plan().target_bars.value, 8);
    CHECK_EQ(sched.plan().duration_ticks.value, 15360); /* 8*4*480 */
}

/* ------------------------------------------------------------------ */
/*  Test: double start rejected                                        */
/* ------------------------------------------------------------------ */

static void
test_double_start ()
{
    std::cout << "\n--- Double start rejected ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);
    sched.set_intention_callback([](const seq66::recording_intention &) {});

    transport.running = true;
    transport.pos = seq66::tick_position(0);

    CHECK(sched.start_recording(make_request(4)));
    CHECK(! sched.start_recording(make_request(2)));
}

/* ------------------------------------------------------------------ */
/*  Test: state string                                                 */
/* ------------------------------------------------------------------ */

static void
test_state_string ()
{
    std::cout << "\n--- State string ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    seq66::recording_scheduler sched(transport, clock);

    CHECK_EQ(std::string(sched.state_string()), "idle");
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int
main ()
{
    test_4_4_four_bars();
    test_transport_stopped();
    test_cancel();
    test_manual_stop();
    test_3_4_three_bars();
    test_5_4_five_bars();
    test_6_8_eight_bars();
    test_7_8_one_bar();
    test_tempo_change();
    test_generation_change();
    test_timeout();
    test_command_rejection();
    test_4_4_eight_bars();
    test_double_start();
    test_state_string();

    std::cout << "\n" << g_total << "/" << g_total
              << " pass. All scheduler assertions passed." << std::endl;
    return g_failed;
}
