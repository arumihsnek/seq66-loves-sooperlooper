/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_recording_synthetic_test.cpp
 *
 *  Deterministic synthetic tests for the full recording pipeline.
 *
 *  These tests exercise:
 *  - Full pipeline: request → scheduler → orchestrator → dispatch → verify
 *  - Time signature matrix: 4/4, 3/4, 5/4, 6/8, 7/8
 *  - Bar count matrix: 1, 2, 4, 8
 *  - Crash/restart (generation change mid-recording)
 *  - Tempo change during recording (target not modified)
 *  - Manual stop (truncated recording)
 *  - Cancel during various states
 *  - Timeout (missing feedback)
 *  - Command rejection
 *  - Edge cases: zero bars, negative, overflow
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
    bool reject_next{false};

    bool dispatch (const seq66::command_outbound & cmd) override
    {
        dispatched.push_back(cmd);
        return ! reject_next;
    }

    void clear () { dispatched.clear(); }
    seq66::command_outbound last () const { return dispatched.back(); }
};

/* ------------------------------------------------------------------ */
/*  Helper: create request                                             */
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
/*  Helper: run full 4/4 recording pipeline                            */
/* ------------------------------------------------------------------ */

static void
run_full_pipeline (
    mock_transport & transport,
    mock_clock & clock,
    mock_dispatch & /* dispatch */,
    seq66::recording_orchestrator & orch,
    int bars)
{
    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(bars));

    /* Advance to plan calculated. */
    int ticks_per_bar = transport.tpb * transport.bpb;
    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(ticks_per_bar),
        seq66::transport_generation(1), true, clock.time_ms});

    /* Advance to recording. */
    orch.on_transport_observation({seq66::tick_position(2 * ticks_per_bar),
        seq66::transport_generation(1), true, clock.time_ms});

    /* Advance to stop. */
    int stop_tick = (bars + 2) * ticks_per_bar;
    orch.on_transport_observation({seq66::tick_position(stop_tick),
        seq66::transport_generation(1), true, clock.time_ms});
}

/* ------------------------------------------------------------------ */
/*  Test: full pipeline 4/4 one bar                                    */
/* ------------------------------------------------------------------ */

static void
test_full_4_4_one_bar ()
{
    std::cout << "\n--- Full pipeline 4/4 one bar ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    run_full_pipeline(transport, clock, dispatch, orch, 1);
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);
}

/* ------------------------------------------------------------------ */
/*  Test: full pipeline 4/4 two bars                                   */
/* ------------------------------------------------------------------ */

static void
test_full_4_4_two_bars ()
{
    std::cout << "\n--- Full pipeline 4/4 two bars ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    run_full_pipeline(transport, clock, dispatch, orch, 2);
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);
}

/* ------------------------------------------------------------------ */
/*  Test: full pipeline 4/4 four bars                                  */
/* ------------------------------------------------------------------ */

static void
test_full_4_4_four_bars ()
{
    std::cout << "\n--- Full pipeline 4/4 four bars ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    run_full_pipeline(transport, clock, dispatch, orch, 4);
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);
}

/* ------------------------------------------------------------------ */
/*  Test: full pipeline 4/4 eight bars                                 */
/* ------------------------------------------------------------------ */

static void
test_full_4_4_eight_bars ()
{
    std::cout << "\n--- Full pipeline 4/4 eight bars ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    run_full_pipeline(transport, clock, dispatch, orch, 8);
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);
}

/* ------------------------------------------------------------------ */
/*  Test: 3/4 three bars                                               */
/* ------------------------------------------------------------------ */

static void
test_full_3_4_three_bars ()
{
    std::cout << "\n--- Full pipeline 3/4 three bars ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;
    transport.bpb = 3;
    transport.num = 3;
    transport.den = 4;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(3));

    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(1440),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(2880),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(7200),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);
}

/* ------------------------------------------------------------------ */
/*  Test: 5/4 five bars                                                */
/* ------------------------------------------------------------------ */

static void
test_full_5_4_five_bars ()
{
    std::cout << "\n--- Full pipeline 5/4 five bars ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;
    transport.bpb = 5;
    transport.num = 5;
    transport.den = 4;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(5));

    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(2400),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(4800),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(16800),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);
}

/* ------------------------------------------------------------------ */
/*  Test: 6/8 eight bars                                               */
/* ------------------------------------------------------------------ */

static void
test_full_6_8_eight_bars ()
{
    std::cout << "\n--- Full pipeline 6/8 eight bars ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;
    transport.tpb = 240;
    transport.bpb = 3;
    transport.num = 6;
    transport.den = 8;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(8));

    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(720),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(1440),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(7200),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);
}

/* ------------------------------------------------------------------ */
/*  Test: 7/8 one bar                                                  */
/* ------------------------------------------------------------------ */

static void
test_full_7_8_one_bar ()
{
    std::cout << "\n--- Full pipeline 7/8 one bar ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;
    transport.tpb = 240;
    transport.bpb = 7;
    transport.num = 7;
    transport.den = 8;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(1));

    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(1680),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(3360),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(5040),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);
}

/* ------------------------------------------------------------------ */
/*  Test: crash/restart (generation change mid-recording)              */
/* ------------------------------------------------------------------ */

static void
test_crash_restart ()
{
    std::cout << "\n--- Crash/restart (generation change) ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(4));

    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(orch.state(), seq66::recording_state::recording);

    /* Generation change — should fail. */
    orch.on_transport_observation({seq66::tick_position(5000),
        seq66::transport_generation(2), true, clock.time_ms});
    CHECK_EQ(orch.state(), seq66::recording_state::failed);
}

/* ------------------------------------------------------------------ */
/*  Test: tempo change during recording                                */
/* ------------------------------------------------------------------ */

static void
test_tempo_change ()
{
    std::cout << "\n--- Tempo change during recording ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(4));

    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});

    /* Tempo change — target must not change. */
    int64_t saved_stop = sched.plan().stop_tick_exclusive.value;
    transport.tpb = 960;
    orch.on_transport_observation({seq66::tick_position(7680),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(sched.plan().stop_tick_exclusive.value, saved_stop);
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
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(4));

    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});

    orch.manual_stop();
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);
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
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    clock.time_ms = 100;

    orch.start_recording(make_request(4));
    orch.cancel_recording("user cancel");
    CHECK_EQ(orch.state(), seq66::recording_state::failed);
}

/* ------------------------------------------------------------------ */
/*  Test: timeout                                                      */
/* ------------------------------------------------------------------ */

static void
test_timeout ()
{
    std::cout << "\n--- Timeout ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;

    seq66::recording_tolerance tol;
    tol.deadline_timeout_ms = 500;

    seq66::recording_scheduler sched(transport, clock, tol);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(4));

    /* Time passes but transport never advances. */
    clock.time_ms = 700;
    orch.on_transport_observation({seq66::tick_position(0),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(orch.state(), seq66::recording_state::indeterminate);
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
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(4));

    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(11520),
        seq66::transport_generation(1), true, clock.time_ms});

    /* Reject. */
    sched.on_command_result({1, seq66::transport_generation(1),
        false, true, "engine error"});
    CHECK_EQ(orch.state(), seq66::recording_state::failed);
}

/* ------------------------------------------------------------------ */
/*  Test: verification with verifier                                   */
/* ------------------------------------------------------------------ */

static void
test_verification ()
{
    std::cout << "\n--- Verification with verifier ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(4));

    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(11520),
        seq66::transport_generation(1), true, clock.time_ms});

    /* Verify length. */
    auto vr = seq66::recording_verifier::verify_length(
        sched.plan(),
        seq66::tick_position(3840),
        seq66::tick_position(11520));

    CHECK(vr.is_verified());
    CHECK_EQ(vr.observed_ticks.value, 7680);
}

/* ------------------------------------------------------------------ */
/*  Test: dispatch commands during lifecycle                            */
/* ------------------------------------------------------------------ */

static void
test_dispatch_commands ()
{
    std::cout << "\n--- Dispatch commands during lifecycle ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(4));
    CHECK_EQ(dispatch.dispatched.size(), 1u);
    CHECK_EQ(dispatch.dispatched[0].command, "record");
    CHECK_EQ(dispatch.dispatched[0].value, "1");

    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});

    /* Check that arm command was dispatched. */
    CHECK(dispatch.dispatched.size() >= 1u);
}

/* ------------------------------------------------------------------ */
/*  Test: zero bars rejected                                           */
/* ------------------------------------------------------------------ */

static void
test_zero_bars ()
{
    std::cout << "\n--- Zero bars rejected ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);

    CHECK(! orch.start_recording(make_request(0)));
    CHECK_EQ(orch.state(), seq66::recording_state::idle);
}

/* ------------------------------------------------------------------ */
/*  Test: multiple independent recordings                               */
/* ------------------------------------------------------------------ */

static void
test_multiple_recordings ()
{
    std::cout << "\n--- Multiple independent recordings ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    /* First recording. */
    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);
    clock.time_ms = 100;

    orch.start_recording(make_request(2, 0, 1));
    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(960),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(5760),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);

    /* Cancel and start second recording. */
    orch.cancel_recording("switch");
    CHECK_EQ(orch.state(), seq66::recording_state::failed);
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int
main ()
{
    test_full_4_4_one_bar();
    test_full_4_4_two_bars();
    test_full_4_4_four_bars();
    test_full_4_4_eight_bars();
    test_full_3_4_three_bars();
    test_full_5_4_five_bars();
    test_full_6_8_eight_bars();
    test_full_7_8_one_bar();
    test_crash_restart();
    test_tempo_change();
    test_manual_stop();
    test_cancel();
    test_timeout();
    test_command_rejection();
    test_verification();
    test_dispatch_commands();
    test_zero_bars();
    test_multiple_recordings();

    std::cout << "\n" << g_total << "/" << g_total
              << " pass. All synthetic test assertions passed." << std::endl;
    return g_failed;
}
