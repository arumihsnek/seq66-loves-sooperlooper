/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_audio_slot_widget_test.cpp
 *
 *  Tests for the audio slot model layer.
 *
 *  Covers:
 *  - Initial state and defaults
 *  - has_clip() and command_terminal()
 *  - state_label(), command_label(), tempo_label()
 *  - action factory methods
 *  - MIDI-only regression: no clip assigned, no state change
 *  - pending/confirmed/failed/indeterminate lifecycle
 *  - tempo mode transitions
 *  - transport actions
 */

#include "audio/sooperlooper_audio_slot_widget.hpp"

#include <cassert>
#include <cstring>
#include <cstdio>

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

/* ------------------------------------------------------------------ */
/*  Initial state                                                     */
/* ------------------------------------------------------------------ */

static int
test_initial_state ()
{
    seq66::audio_slot_model m;

    CHECK_EQ(m.runtime_index, -1);
    CHECK(m.clip_uuid.empty());
    CHECK_EQ(m.observed_state, seq66::loop_state::off);
    CHECK_EQ(m.command_status, seq66::command_feedback::none);
    CHECK_EQ(m.tempo_mode, seq66::tempo_mode_selection::free);
    CHECK(! m.transport_playing);
    CHECK(m.last_error.empty());
    CHECK_EQ(m.last_state_change_ms, 0u);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  has_clip                                                          */
/* ------------------------------------------------------------------ */

static int
test_has_clip_empty ()
{
    seq66::audio_slot_model m;
    CHECK(! m.has_clip());
    return 0;
}

static int
test_has_clip_assigned ()
{
    seq66::audio_slot_model m;
    m.clip_uuid = "uuid-abc-123";
    CHECK(m.has_clip());
    return 0;
}

/* ------------------------------------------------------------------ */
/*  command_terminal                                                  */
/* ------------------------------------------------------------------ */

static int
test_command_terminal_none ()
{
    seq66::audio_slot_model m;
    m.command_status = seq66::command_feedback::none;
    CHECK(! m.command_terminal());
    return 0;
}

static int
test_command_terminal_pending ()
{
    seq66::audio_slot_model m;
    m.command_status = seq66::command_feedback::pending;
    CHECK(! m.command_terminal());
    return 0;
}

static int
test_command_terminal_confirmed ()
{
    seq66::audio_slot_model m;
    m.command_status = seq66::command_feedback::confirmed;
    CHECK(m.command_terminal());
    return 0;
}

static int
test_command_terminal_failed ()
{
    seq66::audio_slot_model m;
    m.command_status = seq66::command_feedback::failed;
    CHECK(m.command_terminal());
    return 0;
}

static int
test_command_terminal_indeterminate ()
{
    seq66::audio_slot_model m;
    m.command_status = seq66::command_feedback::indeterminate;
    CHECK(m.command_terminal());
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Labels                                                            */
/* ------------------------------------------------------------------ */

static int
test_state_labels ()
{
    seq66::audio_slot_model m;

    struct { seq66::loop_state s; const char * l; } cases[] = {
        {seq66::loop_state::off,      "off"},
        {seq66::loop_state::wait,     "wait"},
        {seq66::loop_state::record,   "record"},
        {seq66::loop_state::play,     "play"},
        {seq66::loop_state::overdub,  "overdub"},
        {seq66::loop_state::multiply, "multiply"},
        {seq66::loop_state::insert,   "insert"},
        {seq66::loop_state::replace,  "replace"},
        {seq66::loop_state::revert,   "revert"},
        {seq66::loop_state::pause,    "pause"},
        {seq66::loop_state::scratch,  "scratch"},
        {seq66::loop_state::reverse,  "reverse"},
        {seq66::loop_state::one_shot, "one-shot"},
        {seq66::loop_state::tripped,  "tripped"},
        {seq66::loop_state::stop,     "stop"},
        {seq66::loop_state::unknown,  "unknown"},
    };

    for (const auto & c : cases)
    {
        m.observed_state = c.s;
        CHECK(std::strcmp(m.state_label(), c.l) == 0);
    }
    return 0;
}

static int
test_command_labels ()
{
    seq66::audio_slot_model m;

    struct { seq66::command_feedback s; const char * l; } cases[] = {
        {seq66::command_feedback::none,          "idle"},
        {seq66::command_feedback::pending,       "pending"},
        {seq66::command_feedback::confirmed,     "confirmed"},
        {seq66::command_feedback::failed,        "failed"},
        {seq66::command_feedback::indeterminate, "indeterminate"},
    };

    for (const auto & c : cases)
    {
        m.command_status = c.s;
        CHECK(std::strcmp(m.command_label(), c.l) == 0);
    }
    return 0;
}

static int
test_tempo_labels ()
{
    seq66::audio_slot_model m;

    struct { seq66::tempo_mode_selection s; const char * l; } cases[] = {
        {seq66::tempo_mode_selection::free,    "free"},
        {seq66::tempo_mode_selection::tape,    "tape"},
        {seq66::tempo_mode_selection::elastic, "elastic"},
    };

    for (const auto & c : cases)
    {
        m.tempo_mode = c.s;
        CHECK(std::strcmp(m.tempo_label(), c.l) == 0);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Action factories                                                  */
/* ------------------------------------------------------------------ */

static int
test_action_make_transport ()
{
    auto a = seq66::audio_slot_action::make_transport(
        seq66::transport_action::start);
    CHECK(a.has_transport);
    CHECK(! a.has_tempo);
    CHECK_EQ(a.transport, seq66::transport_action::start);

    auto b = seq66::audio_slot_action::make_transport(
        seq66::transport_action::stop);
    CHECK(b.has_transport);
    CHECK_EQ(b.transport, seq66::transport_action::stop);
    return 0;
}

static int
test_action_make_tempo ()
{
    auto a = seq66::audio_slot_action::make_tempo(
        seq66::tempo_mode_selection::elastic);
    CHECK(! a.has_transport);
    CHECK(a.has_tempo);
    CHECK_EQ(a.tempo, seq66::tempo_mode_selection::elastic);
    return 0;
}

static int
test_action_make_both ()
{
    auto a = seq66::audio_slot_action::make_both(
        seq66::transport_action::start,
        seq66::tempo_mode_selection::tape);
    CHECK(a.has_transport);
    CHECK(a.has_tempo);
    CHECK_EQ(a.transport, seq66::transport_action::start);
    CHECK_EQ(a.tempo, seq66::tempo_mode_selection::tape);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  MIDI-only regression                                              */
/* ------------------------------------------------------------------ */

static int
test_midi_only_no_clip ()
{
    seq66::audio_slot_model m;

    /* No clip assigned, no state change — MIDI-only slot. */
    CHECK(! m.has_clip());
    CHECK_EQ(m.observed_state, seq66::loop_state::off);
    CHECK_EQ(m.command_status, seq66::command_feedback::none);
    CHECK_EQ(m.tempo_mode, seq66::tempo_mode_selection::free);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Command lifecycle                                                 */
/* ------------------------------------------------------------------ */

static int
test_command_lifecycle ()
{
    seq66::audio_slot_model m;

    /* Start: none */
    CHECK_EQ(m.command_status, seq66::command_feedback::none);
    CHECK(! m.command_terminal());

    /* Dispatch → pending */
    m.command_status = seq66::command_feedback::pending;
    CHECK(! m.command_terminal());

    /* Feedback → confirmed */
    m.command_status = seq66::command_feedback::confirmed;
    CHECK(m.command_terminal());

    /* Reset for next command */
    m.command_status = seq66::command_feedback::none;
    CHECK(! m.command_terminal());

    /* Dispatch → pending */
    m.command_status = seq66::command_feedback::pending;

    /* Deadline → indeterminate */
    m.command_status = seq66::command_feedback::indeterminate;
    CHECK(m.command_terminal());

    /* Reset, dispatch, then fail */
    m.command_status = seq66::command_feedback::none;
    m.command_status = seq66::command_feedback::pending;
    m.command_status = seq66::command_feedback::failed;
    CHECK(m.command_terminal());

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Tempo mode transitions                                            */
/* ------------------------------------------------------------------ */

static int
test_tempo_transitions ()
{
    seq66::audio_slot_model m;

    CHECK_EQ(m.tempo_mode, seq66::tempo_mode_selection::free);

    m.tempo_mode = seq66::tempo_mode_selection::tape;
    CHECK_EQ(m.tempo_mode, seq66::tempo_mode_selection::tape);

    m.tempo_mode = seq66::tempo_mode_selection::elastic;
    CHECK_EQ(m.tempo_mode, seq66::tempo_mode_selection::elastic);

    m.tempo_mode = seq66::tempo_mode_selection::free;
    CHECK_EQ(m.tempo_mode, seq66::tempo_mode_selection::free);

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Transport                                                         */
/* ------------------------------------------------------------------ */

static int
test_transport_transitions ()
{
    seq66::audio_slot_model m;

    CHECK(! m.transport_playing);

    m.transport_playing = true;
    CHECK(m.transport_playing);

    m.transport_playing = false;
    CHECK(! m.transport_playing);

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Error message                                                     */
/* ------------------------------------------------------------------ */

static int
test_error_message ()
{
    seq66::audio_slot_model m;

    CHECK(m.last_error.empty());

    m.last_error = "timeout waiting for engine";
    CHECK(! m.last_error.empty());

    m.last_error.clear();
    CHECK(m.last_error.empty());

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Multiple slots independent                                        */
/* ------------------------------------------------------------------ */

static int
test_multiple_slots ()
{
    seq66::audio_slot_model slot_a;
    seq66::audio_slot_model slot_b;

    slot_a.clip_uuid = "uuid-aaa";
    slot_b.clip_uuid = "uuid-bbb";
    slot_a.observed_state = seq66::loop_state::record;
    slot_b.observed_state = seq66::loop_state::play;

    CHECK(slot_a.has_clip());
    CHECK(slot_b.has_clip());
    CHECK_EQ(slot_a.observed_state, seq66::loop_state::record);
    CHECK_EQ(slot_b.observed_state, seq66::loop_state::play);

    slot_a.command_status = seq66::command_feedback::pending;
    CHECK(! slot_a.command_terminal());
    CHECK(! slot_b.command_terminal());

    slot_b.command_status = seq66::command_feedback::confirmed;
    CHECK(! slot_a.command_terminal());
    CHECK(slot_b.command_terminal());

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */

int
main ()
{
    std::printf("=== Audio slot widget model tests ===\n\n");

    struct test { const char * name; int (*fn)(); };
    struct test tests[] = {
        {"initial state",                test_initial_state},
        {"has_clip empty",               test_has_clip_empty},
        {"has_clip assigned",            test_has_clip_assigned},
        {"command_terminal none",        test_command_terminal_none},
        {"command_terminal pending",     test_command_terminal_pending},
        {"command_terminal confirmed",   test_command_terminal_confirmed},
        {"command_terminal failed",      test_command_terminal_failed},
        {"command_terminal indeterminate", test_command_terminal_indeterminate},
        {"state labels",                 test_state_labels},
        {"command labels",               test_command_labels},
        {"tempo labels",                 test_tempo_labels},
        {"action make_transport",        test_action_make_transport},
        {"action make_tempo",            test_action_make_tempo},
        {"action make_both",             test_action_make_both},
        {"MIDI-only no clip",            test_midi_only_no_clip},
        {"command lifecycle",            test_command_lifecycle},
        {"tempo transitions",            test_tempo_transitions},
        {"transport transitions",        test_transport_transitions},
        {"error message",                test_error_message},
        {"multiple slots independent",   test_multiple_slots},
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
