/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_protocol_test.cpp
 *
 *  Focused unit test for the typed SooperLooper protocol identifiers.
 *  Compiles as a headless fake-engine test (no liblo server needed).
 *
 *  Build command (mirrors audio-core.yml):
 *      g++ \\
 *          -std=c++17 \\
 *          -Wall -Wextra -Wpedantic -Werror \\
 *          -pthread \\
 *          -I.ci/include \\
 *          -Ilibseq66/include \\
 *          libseq66/src/audio/audio_clip.cpp \\
 *          libseq66/src/audio/sooperlooper_client.cpp \\
 *          libseq66/src/audio/sooperlooper_protocol.cpp \\
 *          tests/audio/sooperlooper_protocol_test.cpp \\
 *          -llo \\
 *          -o .ci/bin/sooperlooper_protocol_test
 *
 *  Run: .ci/bin/sooperlooper_protocol_test
 */

#include <cassert>
#include <cmath>
#include <limits>
#include <string>

#include "audio/audio_clip.hpp"
#include "audio/sooperlooper_protocol.hpp"

using namespace seq66;

/* -------------------------------------------------------------------------
 *  sooperlooper_command tests
 * ------------------------------------------------------------------------- */

static void test_command_strings ()
{
    assert(std::string(to_string(sooperlooper_command::record)) == "record");
    assert(std::string(to_string(sooperlooper_command::overdub)) == "overdub");
    assert(std::string(to_string(sooperlooper_command::multiply)) == "multiply");
    assert(std::string(to_string(sooperlooper_command::insert)) == "insert");
    assert(std::string(to_string(sooperlooper_command::replace)) == "replace");
    assert(std::string(to_string(sooperlooper_command::reverse)) == "reverse");
    assert(std::string(to_string(sooperlooper_command::mute)) == "mute");
    assert(std::string(to_string(sooperlooper_command::undo)) == "undo");
    assert(std::string(to_string(sooperlooper_command::redo)) == "redo");
    assert(std::string(to_string(sooperlooper_command::one_shot)) == "oneshot");
    assert(std::string(to_string(sooperlooper_command::trigger)) == "trigger");
    assert(std::string(to_string(sooperlooper_command::substitute)) == "substitute");
    assert(std::string(to_string(sooperlooper_command::pause)) == "pause");
    assert(std::string(to_string(sooperlooper_command::solo)) == "solo");
    assert(std::string(to_string(sooperlooper_command::mute_on)) == "mute_on");
    assert(std::string(to_string(sooperlooper_command::mute_off)) == "mute_off");
}

/* -------------------------------------------------------------------------
 *  loop_control string mappings, metadata and range validation
 * ------------------------------------------------------------------------- */

static void test_loop_control_strings_and_ranges ()
{
    struct TestCase {
        loop_control ctrl;
        const char * expected;
        float valid;
        float invalid_low;
        float invalid_high;
    };

    TestCase cases[] = {
        { loop_control::rec_thresh,        "rec_thresh",        0.5f, -0.1f, 1.1f },
        { loop_control::feedback,          "feedback",          0.5f, -0.1f, 1.1f },
        { loop_control::use_feedback_play, "use_feedback_play", 0.5f, -0.1f, 1.1f },
        { loop_control::dry,               "dry",               0.5f, -0.1f, 1.1f },
        { loop_control::wet,               "wet",               0.5f, -0.1f, 1.1f },
        { loop_control::input_gain,        "input_gain",        0.5f, -0.1f, 1.1f },
        { loop_control::use_safety_feedback, "use_safety_feedback", 0.5f, -0.1f, 1.1f },
        { loop_control::rate,              "rate",              2.0f, -0.1f, 4.1f },
        { loop_control::use_rate,          "use_rate",          0.5f, -0.1f, 1.1f },
        { loop_control::stretch_ratio,     "stretch_ratio",     2.0f, -0.1f, 4.1f },
        { loop_control::pitch_shift,       "pitch_shift",       5.0f, -24.1f, 24.1f },
        { loop_control::tempo_stretch,     "tempo_stretch",     0.5f, -0.1f, 1.1f },
        { loop_control::round_integer_tempo, "round_integer_tempo", 0.5f, -0.1f, 1.1f },
        { loop_control::quantize,          "quantize",          0.5f, -0.1f, 4.1f },
        { loop_control::round,             "round",             0.0f, -0.1f, 1.1f },
        { loop_control::sync,              "sync",              1.0f, -0.1f, 1.1f },
        { loop_control::playback_sync,     "playback_sync",     1.0f, -0.1f, 1.1f },
        { loop_control::relative_sync,     "relative_sync",     1.0f, -0.1f, 1.1f },
        { loop_control::mute_quantized,    "mute_quantized",    0.5f, -0.1f, 1.1f },
        { loop_control::overdub_quantized, "overdub_quantized", 0.5f, -0.1f, 1.1f },
        { loop_control::replace_quantized, "replace_quantized", 0.5f, -0.1f, 1.1f },
        { loop_control::redo_is_tap,       "redo_is_tap",       0.5f, -0.1f, 1.1f },
        { loop_control::jack_timebase_master, "jack_timebase_master", 0.5f, -0.1f, 1.1f },
        { loop_control::fade_samples,      "fade_samples",      100.0f, -1.0f, 4096.1f },
        { loop_control::input_latency,     "input_latency",     1000.0f, -1.0f, 32768.1f },
        { loop_control::output_latency,    "output_latency",    1000.0f, -1.0f, 32768.1f },
        { loop_control::trigger_latency,   "trigger_latency",   1000.0f, -1.0f, 32768.1f },
        { loop_control::autoset_latency,   "autoset_latency",   1.0f, -1.0f, 32768.1f },
        { loop_control::discrete_prefader, "discrete_prefader", 0.5f, -0.1f, 1.1f },
        { loop_control::use_common_ins,    "use_common_ins",    0.5f, -0.1f, 1.1f },
        { loop_control::use_common_outs,   "use_common_outs",   0.5f, -0.1f, 1.1f },
        { loop_control::pan_1,             "pan_1",             0.0f, -1.1f, 1.1f },
        { loop_control::pan_2,             "pan_2",             0.0f, -1.1f, 1.1f },
        { loop_control::pan_3,             "pan_3",             0.0f, -1.1f, 1.1f },
        { loop_control::pan_4,             "pan_4",             0.0f, -1.1f, 1.1f },
        { loop_control::scratch_pos,       "scratch_pos",       0.5f, -0.1f, 1.1f },
        { loop_control::delay_trigger,     "delay_trigger",     0.5f, -0.1f, 1.1f },
        { loop_control::waiting,           "waiting",           0.5f, -0.1f, 1.1f },
        { loop_control::state,             "state",             0.5f, -0.1f, 1.1f },
        { loop_control::next_state,        "next_state",        0.5f, -0.1f, 1.1f },
        { loop_control::loop_len,          "loop_len",          4.5f, -0.1f, std::numeric_limits<float>::infinity() },
        { loop_control::loop_pos,          "loop_pos",          0.5f, -0.1f, 1.1f },
        { loop_control::cycle_len,         "cycle_len",         4.5f, -0.1f, std::numeric_limits<float>::infinity() },
        { loop_control::free_time,         "free_time",         4.5f, -0.1f, std::numeric_limits<float>::infinity() },
        { loop_control::total_time,        "total_time",        4.5f, -0.1f, std::numeric_limits<float>::infinity() },
        { loop_control::rate_output,       "rate_output",       44100.0f, -0.1f, std::numeric_limits<float>::infinity() },
        { loop_control::has_discrete_io,   "has_discrete_io",   0.5f, -0.1f, 1.1f },
        { loop_control::channel_count,     "channel_count",     2.0f, 0.0f, 16.1f },
        { loop_control::in_peak_meter,     "in_peak_meter",     0.5f, -0.1f, 1.1f },
        { loop_control::out_peak_meter,    "out_peak_meter",    0.5f, -0.1f, 1.1f },
        { loop_control::is_soloed,         "is_soloed",         0.5f, -0.1f, 1.1f }
    };

    for (std::size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i)
    {
        auto & c = cases[i];
        assert(std::string(to_string(c.ctrl)) == c.expected);
        assert(is_in_range(c.ctrl, c.valid));
        assert(!is_in_range(c.ctrl, c.invalid_low));
        assert(!is_in_range(c.ctrl, c.invalid_high));
        assert(!is_in_range(c.ctrl, INFINITY));
        assert(!is_in_range(c.ctrl, -INFINITY));
        assert(!is_in_range(c.ctrl, std::numeric_limits<float>::quiet_NaN()));
    }

    // test try_parse round-trip
    for (std::size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i)
    {
        loop_control ctrl;
        if (! try_parse(cases[i].expected, ctrl))
        {
            assert(false && "try_parse failed for known control");
        }
        if (ctrl != cases[i].ctrl)
        {
            assert(false && "try_parse round-trip mismatch");
        }
    }
    // unknown string returns false and leaves output unchanged
    loop_control unchanged = loop_control::rec_thresh;
    assert(!try_parse("totally_fake_control", unchanged));
    assert(unchanged == loop_control::rec_thresh);
}

/* -------------------------------------------------------------------------
 *  global_control tests
 * ------------------------------------------------------------------------- */

static void test_global_control_strings_and_ranges ()
{
    struct TestCase {
        global_control ctrl;
        const char * expected;
        float valid;
        float invalid_low;
        float invalid_high;
    };

    TestCase cases[] = {
        { global_control::tempo,                  "tempo",                  120.0f, -1.0f, 1001.0f },
        { global_control::eighth_per_cycle,       "eighth_per_cycle",       128.0f, -1.0f, 2049.0f },
        { global_control::sync_source,            "sync_source",            0.0f, -4.0f, 1.0f },   // -3..0 valid, 1 invalid
        { global_control::tap_tempo,              "tap_tempo",              120.0f, -1.0f, 1001.0f },
        { global_control::save_loop,              "save_loop",              0.5f, -0.1f, 1.1f },
        { global_control::auto_disable_latency,   "auto_disable_latency",   1.0f, -0.1f, 1.1f },
        { global_control::select_next_loop,       "select_next_loop",       0.5f, -0.1f, 1.1f },
        { global_control::select_prev_loop,       "select_prev_loop",       0.5f, -0.1f, 1.1f },
        { global_control::select_all_loops,       "select_all_loops",       0.5f, -0.1f, 1.1f },
        { global_control::selected_loop_num,      "selected_loop_num",      3.0f, -1.0f, 5.0f },   // -1..N
        { global_control::output_midi_clock,      "output_midi_clock",      0.0f, -0.1f, 1.1f },
        { global_control::smart_eighths,          "smart_eighths",          1.0f, -0.1f, 1.1f },
        { global_control::use_midi_start,         "use_midi_start",         0.0f, -0.1f, 1.1f },
        { global_control::use_midi_stop,          "use_midi_stop",          1.0f, -0.1f, 1.1f },
        { global_control::send_midi_start_on_trigger, "send_midi_start_on_trigger", 0.0f, -0.1f, 1.1f },
        { global_control::global_cycle_len,       "global_cycle_len",       100.0f, -0.1f, std::numeric_limits<float>::infinity() },
        { global_control::global_cycle_pos,       "global_cycle_pos",       50.0f, -0.1f, std::numeric_limits<float>::infinity() }
    };

    for (auto & c : cases)
    {
        assert(std::string(to_string(c.ctrl)) == c.expected);
        assert(is_in_range(c.ctrl, c.valid));
        assert(!is_in_range(c.ctrl, c.invalid_low));
        assert(!is_in_range(c.ctrl, c.invalid_high));
        assert(!is_in_range(c.ctrl, INFINITY));
        assert(!is_in_range(c.ctrl, -INFINITY));
        assert(!is_in_range(c.ctrl, std::numeric_limits<float>::quiet_NaN()));
    }

    // test try_parse round-trip
    for (std::size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i)
    {
        global_control ctrl;
        assert(try_parse(cases[i].expected, ctrl));
        assert(ctrl == cases[i].ctrl);
    }
    // unknown string
    global_control gc = global_control::tempo;
    assert(!try_parse("not_a_global_control", gc));
    assert(gc == global_control::tempo);
}

/* -------------------------------------------------------------------------
 *  payload_kind metadata tests
 * ------------------------------------------------------------------------- */

static void test_metadata ()
{
    assert(metadata_for(loop_control::rec_thresh) == payload_kind::ratio);
    assert(metadata_for(loop_control::feedback) == payload_kind::ratio);
    assert(metadata_for(loop_control::use_feedback_play) == payload_kind::ratio);
    assert(metadata_for(loop_control::dry) == payload_kind::ratio);
    assert(metadata_for(loop_control::wet) == payload_kind::ratio);
    assert(metadata_for(loop_control::input_gain) == payload_kind::ratio);
    assert(metadata_for(loop_control::use_safety_feedback) == payload_kind::ratio);
    assert(metadata_for(loop_control::rate) == payload_kind::ratio);
    assert(metadata_for(loop_control::use_rate) == payload_kind::ratio);
    assert(metadata_for(loop_control::stretch_ratio) == payload_kind::ratio);
    assert(metadata_for(loop_control::pitch_shift) == payload_kind::ratio);
    assert(metadata_for(loop_control::tempo_stretch) == payload_kind::ratio);
    assert(metadata_for(loop_control::round_integer_tempo) == payload_kind::boolean);
    assert(metadata_for(loop_control::quantize) == payload_kind::indexed_quantize);
    assert(metadata_for(loop_control::round) == payload_kind::ratio);
    assert(metadata_for(loop_control::sync) == payload_kind::ratio);
    assert(metadata_for(loop_control::playback_sync) == payload_kind::ratio);
    assert(metadata_for(loop_control::relative_sync) == payload_kind::ratio);
    assert(metadata_for(loop_control::mute_quantized) == payload_kind::ratio);
    assert(metadata_for(loop_control::overdub_quantized) == payload_kind::ratio);
    assert(metadata_for(loop_control::replace_quantized) == payload_kind::ratio);
    assert(metadata_for(loop_control::redo_is_tap) == payload_kind::ratio);
    assert(metadata_for(loop_control::jack_timebase_master) == payload_kind::ratio);
    assert(metadata_for(loop_control::fade_samples) == payload_kind::seconds);
    assert(metadata_for(loop_control::input_latency) == payload_kind::seconds);
    assert(metadata_for(loop_control::output_latency) == payload_kind::seconds);
    assert(metadata_for(loop_control::trigger_latency) == payload_kind::seconds);
    assert(metadata_for(loop_control::autoset_latency) == payload_kind::seconds);
    assert(metadata_for(loop_control::discrete_prefader) == payload_kind::ratio);
    assert(metadata_for(loop_control::use_common_ins) == payload_kind::ratio);
    assert(metadata_for(loop_control::use_common_outs) == payload_kind::ratio);
    assert(metadata_for(loop_control::pan_1) == payload_kind::ratio);
    assert(metadata_for(loop_control::pan_2) == payload_kind::ratio);
    assert(metadata_for(loop_control::pan_3) == payload_kind::ratio);
    assert(metadata_for(loop_control::pan_4) == payload_kind::ratio);
    assert(metadata_for(loop_control::scratch_pos) == payload_kind::ratio);
    assert(metadata_for(loop_control::delay_trigger) == payload_kind::ratio);
    assert(metadata_for(loop_control::waiting) == payload_kind::ratio);
    assert(metadata_for(loop_control::state) == payload_kind::ratio);
    assert(metadata_for(loop_control::next_state) == payload_kind::ratio);
    assert(metadata_for(loop_control::loop_len) == payload_kind::seconds);
    assert(metadata_for(loop_control::loop_pos) == payload_kind::ratio);
    assert(metadata_for(loop_control::cycle_len) == payload_kind::seconds);
    assert(metadata_for(loop_control::free_time) == payload_kind::seconds);
    assert(metadata_for(loop_control::total_time) == payload_kind::seconds);
    assert(metadata_for(loop_control::rate_output) == payload_kind::seconds);
    assert(metadata_for(loop_control::has_discrete_io) == payload_kind::boolean);
    assert(metadata_for(loop_control::channel_count) == payload_kind::seconds);
    assert(metadata_for(loop_control::in_peak_meter) == payload_kind::ratio);
    assert(metadata_for(loop_control::out_peak_meter) == payload_kind::ratio);
    assert(metadata_for(loop_control::is_soloed) == payload_kind::ratio);

    assert(metadata_for(global_control::tempo) == payload_kind::tempo_bpm);
    assert(metadata_for(global_control::eighth_per_cycle) == payload_kind::eighths_per_cycle);
    assert(metadata_for(global_control::sync_source) == payload_kind::sync_source_index);
    assert(metadata_for(global_control::tap_tempo) == payload_kind::tempo_bpm);
    assert(metadata_for(global_control::save_loop) == payload_kind::boolean);
    assert(metadata_for(global_control::auto_disable_latency) == payload_kind::boolean);
    assert(metadata_for(global_control::select_next_loop) == payload_kind::boolean);
    assert(metadata_for(global_control::select_prev_loop) == payload_kind::boolean);
    assert(metadata_for(global_control::select_all_loops) == payload_kind::boolean);
    assert(metadata_for(global_control::selected_loop_num) == payload_kind::integer_index);
    assert(metadata_for(global_control::output_midi_clock) == payload_kind::boolean);
    assert(metadata_for(global_control::smart_eighths) == payload_kind::boolean);
    assert(metadata_for(global_control::use_midi_start) == payload_kind::boolean);
    assert(metadata_for(global_control::use_midi_stop) == payload_kind::boolean);
    assert(metadata_for(global_control::send_midi_start_on_trigger) == payload_kind::boolean);
    assert(metadata_for(global_control::global_cycle_len) == payload_kind::seconds);
    assert(metadata_for(global_control::global_cycle_pos) == payload_kind::ratio);
}

/* -------------------------------------------------------------------------
 *  state integer parsing tests
 * ------------------------------------------------------------------------- */

static void test_state_parsing ()
{
    struct TestCase {
        int raw;
        const char * expected_label;
        bool known;
    };

    TestCase cases[] = {
        { -1, "unknown", true },
        {  0, "off", true },
        {  1, "wait_start", true },
        {  2, "recording", true },
        {  3, "wait_stop", true },
        {  4, "playing", true },
        {  5, "overdubbing", true },
        {  6, "multiplying", true },
        {  7, "inserting", true },
        {  8, "replacing", true },
        {  9, "delay", true },
        { 10, "muted", true },
        { 11, "scratching", true },
        { 12, "one_shot", true },
        { 13, "substituting", true },
        { 14, "paused", true },
        { 20, "off_muted", true },
        { 999, nullptr, false },   // unknown future value
        { -5, nullptr, false }     // negative unknown
    };

    for (auto & c : cases)
    {
        state_parse_result result;
        parse_state_int(c.raw, result);
        assert(result.known == c.known);
        if (c.known)
            assert(std::string(result.label) == c.expected_label);
        assert(result.raw == c.raw);
    }
}

/* -------------------------------------------------------------------------
 *  Main
 * ------------------------------------------------------------------------- */

int main ()
{
    test_command_strings();
    test_loop_control_strings_and_ranges();
    test_global_control_strings_and_ranges();
    test_metadata();
    test_state_parsing();
    return 0;
}

/*
 * sooperlooper_protocol_test.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
*/