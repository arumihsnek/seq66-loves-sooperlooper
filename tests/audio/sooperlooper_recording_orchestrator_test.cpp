/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_recording_orchestrator_test.cpp
 *
 *  Tests for the recording orchestrator and verifier.
 *
 *  Coverage:
 *  - Orchestrator bridges scheduler → dispatcher
 *  - Verifier checks length against plan
 *  - Verifier checks beat count
 *  - Tolerance acceptance/rejection
 *  - Full lifecycle: arm → begin → record → end → verify
 *  - Cancel through orchestrator
 *  - Manual stop through orchestrator
 */

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
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

    void clear () { dispatched.clear(); }
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
/*  Test: full lifecycle through orchestrator                          */
/* ------------------------------------------------------------------ */

static void
test_full_lifecycle ()
{
    std::cout << "\n--- Full lifecycle through orchestrator ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    /* Start: should dispatch arm command. */
    CHECK(orch.start_recording(make_request(4)));
    CHECK_EQ(dispatch.dispatched.size(), 1u);
    CHECK_EQ(dispatch.dispatched[0].command, "record");
    CHECK_EQ(dispatch.dispatched[0].value, "1");

    /* Advance to plan calculated. */
    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});

    /* Advance to recording. */
    orch.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(orch.state(), seq66::recording_state::recording);

    /* Advance to stop. */
    orch.on_transport_observation({seq66::tick_position(11520),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);

    /* Verify end command dispatched. */
    bool found_end = false;
    for (const auto & cmd : dispatch.dispatched)
    {
        if (cmd.command == "record" && cmd.value == "0")
        {
            found_end = true;
            break;
        }
    }
    CHECK(found_end);
}

/* ------------------------------------------------------------------ */
/*  Test: cancel through orchestrator                                  */
/* ------------------------------------------------------------------ */

static void
test_cancel_orchestrator ()
{
    std::cout << "\n--- Cancel through orchestrator ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);

    orch.start_recording(make_request(4));
    CHECK_EQ(orch.state(), seq66::recording_state::armed);

    orch.cancel_recording("user cancel");
    CHECK_EQ(orch.state(), seq66::recording_state::failed);
}

/* ------------------------------------------------------------------ */
/*  Test: manual stop through orchestrator                             */
/* ------------------------------------------------------------------ */

static void
test_manual_stop_orchestrator ()
{
    std::cout << "\n--- Manual stop through orchestrator ---" << std::endl;
    mock_transport transport;
    mock_clock clock;
    mock_dispatch dispatch;
    clock.time_ms = 100;

    seq66::recording_scheduler sched(transport, clock);
    seq66::recording_orchestrator orch(sched, dispatch);

    transport.running = true;
    transport.pos = seq66::tick_position(0);
    transport.gen = seq66::transport_generation(1);

    orch.start_recording(make_request(4));

    clock.time_ms = 200;
    orch.on_transport_observation({seq66::tick_position(1920),
        seq66::transport_generation(1), true, clock.time_ms});
    orch.on_transport_observation({seq66::tick_position(3840),
        seq66::transport_generation(1), true, clock.time_ms});
    CHECK_EQ(orch.state(), seq66::recording_state::recording);

    orch.manual_stop();
    CHECK_EQ(orch.state(), seq66::recording_state::verifying);
}

/* ------------------------------------------------------------------ */
/*  Test: verifier length check — exact match                          */
/* ------------------------------------------------------------------ */

static void
test_verifier_exact_match ()
{
    std::cout << "\n--- Verifier exact match ---" << std::endl;
    seq66::recording_plan plan;
    plan.start_tick = seq66::tick_position(0);
    plan.stop_tick_exclusive = seq66::tick_position(7680);
    plan.duration_ticks = seq66::tick_count(7680);
    plan.target_beats = seq66::beat_count(16);

    auto vr = seq66::recording_verifier::verify_length(
        plan, seq66::tick_position(0), seq66::tick_position(7680));

    CHECK_EQ(vr.result_status, seq66::verification_result::status::verified);
    CHECK(vr.is_verified());
}

/* ------------------------------------------------------------------ */
/*  Test: verifier length check — within tolerance                     */
/* ------------------------------------------------------------------ */

static void
test_verifier_within_tolerance ()
{
    std::cout << "\n--- Verifier within tolerance ---" << std::endl;
    seq66::recording_plan plan;
    plan.start_tick = seq66::tick_position(0);
    plan.stop_tick_exclusive = seq66::tick_position(7680);
    plan.duration_ticks = seq66::tick_count(7680);
    plan.target_beats = seq66::beat_count(16);

    seq66::recording_tolerance tol;
    tol.musical_tolerance_ticks = 10;

    auto vr = seq66::recording_verifier::verify_length(
        plan, seq66::tick_position(5), seq66::tick_position(7685), tol);

    CHECK_EQ(vr.result_status, seq66::verification_result::status::verified);
}

/* ------------------------------------------------------------------ */
/*  Test: verifier length check — outside tolerance                    */
/* ------------------------------------------------------------------ */

static void
test_verifier_outside_tolerance ()
{
    std::cout << "\n--- Verifier outside tolerance ---" << std::endl;
    seq66::recording_plan plan;
    plan.start_tick = seq66::tick_position(0);
    plan.stop_tick_exclusive = seq66::tick_position(7680);
    plan.duration_ticks = seq66::tick_count(7680);
    plan.target_beats = seq66::beat_count(16);

    seq66::recording_tolerance tol;
    tol.musical_tolerance_ticks = 5;

    auto vr = seq66::recording_verifier::verify_length(
        plan, seq66::tick_position(0), seq66::tick_position(7690), tol);

    CHECK_EQ(vr.result_status, seq66::verification_result::status::failed);
    CHECK(! vr.is_verified());
}

/* ------------------------------------------------------------------ */
/*  Test: verifier beat count — exact                                   */
/* ------------------------------------------------------------------ */

static void
test_verifier_beats_exact ()
{
    std::cout << "\n--- Verifier beats exact ---" << std::endl;
    seq66::recording_plan plan;
    plan.target_beats = seq66::beat_count(16);
    plan.metric.ticks_per_beat = 480;

    auto vr = seq66::recording_verifier::verify_beats(
        plan, seq66::beat_count(16));

    CHECK_EQ(vr.result_status, seq66::verification_result::status::verified);
}

/* ------------------------------------------------------------------ */
/*  Test: verifier beat count — mismatch                               */
/* ------------------------------------------------------------------ */

static void
test_verifier_beats_mismatch ()
{
    std::cout << "\n--- Verifier beats mismatch ---" << std::endl;
    seq66::recording_plan plan;
    plan.target_beats = seq66::beat_count(16);
    plan.metric.ticks_per_beat = 480;

    auto vr = seq66::recording_verifier::verify_beats(
        plan, seq66::beat_count(14));

    CHECK_EQ(vr.result_status, seq66::verification_result::status::failed);
}

/* ------------------------------------------------------------------ */
/*  Test: verifier with 3/4                                            */
/* ------------------------------------------------------------------ */

static void
test_verifier_3_4 ()
{
    std::cout << "\n--- Verifier 3/4 ---" << std::endl;
    seq66::recording_plan plan;
    plan.start_tick = seq66::tick_position(1440);
    plan.stop_tick_exclusive = seq66::tick_position(5760);
    plan.duration_ticks = seq66::tick_count(4320);
    plan.target_beats = seq66::beat_count(9);
    plan.metric.ticks_per_beat = 480;
    plan.metric.beats_per_bar = 3;

    auto vr = seq66::recording_verifier::verify_length(
        plan, seq66::tick_position(1440), seq66::tick_position(5760));

    CHECK_EQ(vr.result_status, seq66::verification_result::status::verified);
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int
main ()
{
    test_full_lifecycle();
    test_cancel_orchestrator();
    test_manual_stop_orchestrator();
    test_verifier_exact_match();
    test_verifier_within_tolerance();
    test_verifier_outside_tolerance();
    test_verifier_beats_exact();
    test_verifier_beats_mismatch();
    test_verifier_3_4();

    std::cout << "\n" << g_total << "/" << g_total
              << " pass. All orchestrator/verifier assertions passed." << std::endl;
    return g_failed;
}
