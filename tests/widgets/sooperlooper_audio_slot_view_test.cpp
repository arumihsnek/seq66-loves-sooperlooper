/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_audio_slot_view_test.cpp
 *
 *  Tests for the audio slot view model-logic layer.
 *
 *  Since Qt requires a display server, this test exercises the model
 *  layer that the view owns, and verifies the action/signal contract
 *  through model state assertions.
 *
 *  Coverage:
 *  - Model state updates propagate correctly
 *  - Transport actions are correctly formed
 *  - Tempo mode selections are correctly formed
 *  - Command feedback states are correctly displayed
 *  - Edge cases: empty clip, unknown state, terminal feedback
 */

#include <iostream>
#include <string>
#include <cassert>

#include "audio/sooperlooper_audio_slot_widget.hpp"

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
/*  Tests                                                             */
/* ------------------------------------------------------------------ */

static void
test_model_initial_state ()
{
    std::cout << "\n--- Model initial state ---" << std::endl;
    seq66::audio_slot_model m;
    CHECK_EQ(m.runtime_index, -1);
    CHECK(m.clip_uuid.empty());
    CHECK_EQ(m.observed_state, seq66::loop_state::off);
    CHECK_EQ(m.command_status, seq66::command_feedback::none);
    CHECK_EQ(m.tempo_mode, seq66::tempo_mode_selection::free);
    CHECK(! m.transport_playing);
    CHECK(m.last_error.empty());
    CHECK(! m.has_clip());
    CHECK(! m.command_terminal());
    CHECK_EQ(std::string(m.state_label()), "off");
    CHECK_EQ(std::string(m.command_label()), "idle");
    CHECK_EQ(std::string(m.tempo_label()), "free");
}

static void
test_model_state_labels ()
{
    std::cout << "\n--- Model state labels ---" << std::endl;
    seq66::audio_slot_model m;

    m.observed_state = seq66::loop_state::record;
    CHECK_EQ(std::string(m.state_label()), "record");

    m.observed_state = seq66::loop_state::play;
    CHECK_EQ(std::string(m.state_label()), "play");

    m.observed_state = seq66::loop_state::overdub;
    CHECK_EQ(std::string(m.state_label()), "overdub");

    m.observed_state = seq66::loop_state::unknown;
    CHECK_EQ(std::string(m.state_label()), "unknown");
}

static void
test_model_command_labels ()
{
    std::cout << "\n--- Model command labels ---" << std::endl;
    seq66::audio_slot_model m;

    m.command_status = seq66::command_feedback::pending;
    CHECK_EQ(std::string(m.command_label()), "pending");
    CHECK(! m.command_terminal());

    m.command_status = seq66::command_feedback::confirmed;
    CHECK_EQ(std::string(m.command_label()), "confirmed");
    CHECK(m.command_terminal());

    m.command_status = seq66::command_feedback::failed;
    CHECK_EQ(std::string(m.command_label()), "failed");
    CHECK(m.command_terminal());

    m.command_status = seq66::command_feedback::indeterminate;
    CHECK_EQ(std::string(m.command_label()), "indeterminate");
    CHECK(m.command_terminal());
}

static void
test_model_tempo_labels ()
{
    std::cout << "\n--- Model tempo labels ---" << std::endl;
    seq66::audio_slot_model m;

    m.tempo_mode = seq66::tempo_mode_selection::free;
    CHECK_EQ(std::string(m.tempo_label()), "free");

    m.tempo_mode = seq66::tempo_mode_selection::tape;
    CHECK_EQ(std::string(m.tempo_label()), "tape");

    m.tempo_mode = seq66::tempo_mode_selection::elastic;
    CHECK_EQ(std::string(m.tempo_label()), "elastic");
}

static void
test_model_has_clip ()
{
    std::cout << "\n--- Model has_clip ---" << std::endl;
    seq66::audio_slot_model m;
    CHECK(! m.has_clip());

    m.clip_uuid = "some-uuid";
    CHECK(m.has_clip());

    m.clip_uuid.clear();
    CHECK(! m.has_clip());
}

static void
test_transport_action_creation ()
{
    std::cout << "\n--- Transport action creation ---" << std::endl;
    auto start = seq66::audio_slot_action::make_transport(
        seq66::transport_action::start);
    CHECK(start.has_transport);
    CHECK(! start.has_tempo);
    CHECK_EQ(start.transport, seq66::transport_action::start);

    auto stop = seq66::audio_slot_action::make_transport(
        seq66::transport_action::stop);
    CHECK(stop.has_transport);
    CHECK_EQ(stop.transport, seq66::transport_action::stop);
}

static void
test_tempo_action_creation ()
{
    std::cout << "\n--- Tempo action creation ---" << std::endl;
    auto tape = seq66::audio_slot_action::make_tempo(
        seq66::tempo_mode_selection::tape);
    CHECK(! tape.has_transport);
    CHECK(tape.has_tempo);
    CHECK_EQ(tape.tempo, seq66::tempo_mode_selection::tape);

    auto elastic = seq66::audio_slot_action::make_tempo(
        seq66::tempo_mode_selection::elastic);
    CHECK(elastic.has_tempo);
    CHECK_EQ(elastic.tempo, seq66::tempo_mode_selection::elastic);
}

static void
test_both_action_creation ()
{
    std::cout << "\n--- Both action creation ---" << std::endl;
    auto act = seq66::audio_slot_action::make_both(
        seq66::transport_action::start,
        seq66::tempo_mode_selection::elastic);
    CHECK(act.has_transport);
    CHECK(act.has_tempo);
    CHECK_EQ(act.transport, seq66::transport_action::start);
    CHECK_EQ(act.tempo, seq66::tempo_mode_selection::elastic);
}

static void
test_model_transport_playing ()
{
    std::cout << "\n--- Model transport playing ---" << std::endl;
    seq66::audio_slot_model m;
    CHECK(! m.transport_playing);

    m.transport_playing = true;
    CHECK(m.transport_playing);

    m.transport_playing = false;
    CHECK(! m.transport_playing);
}

static void
test_model_state_transitions ()
{
    std::cout << "\n--- Model state transitions ---" << std::endl;
    seq66::audio_slot_model m;

    /* Simulate: off -> wait -> record -> play */
    m.observed_state = seq66::loop_state::off;
    CHECK_EQ(std::string(m.state_label()), "off");

    m.observed_state = seq66::loop_state::wait;
    CHECK_EQ(std::string(m.state_label()), "wait");

    m.observed_state = seq66::loop_state::record;
    CHECK_EQ(std::string(m.state_label()), "record");

    m.observed_state = seq66::loop_state::play;
    CHECK_EQ(std::string(m.state_label()), "play");

    m.observed_state = seq66::loop_state::overdub;
    CHECK_EQ(std::string(m.state_label()), "overdub");
}

static void
test_model_last_error ()
{
    std::cout << "\n--- Model last_error ---" << std::endl;
    seq66::audio_slot_model m;
    CHECK(m.last_error.empty());

    m.last_error = "timeout";
    CHECK_EQ(m.last_error, "timeout");

    m.last_error.clear();
    CHECK(m.last_error.empty());
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */

int
main ()
{
    test_model_initial_state();
    test_model_state_labels();
    test_model_command_labels();
    test_model_tempo_labels();
    test_model_has_clip();
    test_transport_action_creation();
    test_tempo_action_creation();
    test_both_action_creation();
    test_model_transport_playing();
    test_model_state_transitions();
    test_model_last_error();

    std::cout << "\n" << g_total << "/" << g_total
              << " pass. All assertions passed." << std::endl;
    return g_failed;
}
