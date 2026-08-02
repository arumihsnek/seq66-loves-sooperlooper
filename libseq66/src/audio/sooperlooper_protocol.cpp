/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_protocol.cpp
 *
 *  Central implementation of typed SooperLooper OSC protocol identifiers,
 *  payload kinds, ranges and bidirectional string mappings.
 *
 *  This translation unit is the single source of truth for OSC string
 *  identifiers used by the fork.  Integration code MUST NOT introduce new
 *  raw protocol string literals; it must use the enums and accessors
 *  declared in `sooperlooper_protocol.hpp`.
 *
 *  Coverage is intentionally limited to identifiers already used by the
 *  outbound adapter and documented in
 *  `doc/sooperlooper/OSC-CONTROL-AND-FEEDBACK.md`.  Unknown identifiers
 *  are rejected on the outbound side and preserved as raw integers on
 *  the inbound side, per spec.
 */

#include <array>
#include <cmath>
#include <string>
#include <utility>

#include "audio/sooperlooper_protocol.hpp"

namespace seq66
{

namespace
{

/**
 *  Range inclusivity flags.  Inclusive bounds use `>=` / `<=`, exclusive
 *  bounds use `>` / `<` (the spec allows sentinel values at the boundary,
 *  e.g. `selected_loop_num == -1` means "no selection").
 */
enum class bound_mode : int
{
    inclusive = 0,
    exclusive = 1
};

}               // anonymous namespace

/* -------------------------------------------------------------------------
 *  sooperlooper_command  (sent via /sl/<index>/hit)
 * ------------------------------------------------------------------------- */

const char * to_string (sooperlooper_command command)
{
    switch (command)
    {
        case sooperlooper_command::record:     return "record";
        case sooperlooper_command::overdub:    return "overdub";
        case sooperlooper_command::multiply:   return "multiply";
        case sooperlooper_command::insert:     return "insert";
        case sooperlooper_command::replace:    return "replace";
        case sooperlooper_command::reverse:    return "reverse";
        case sooperlooper_command::mute:       return "mute";
        case sooperlooper_command::undo:       return "undo";
        case sooperlooper_command::redo:       return "redo";
        case sooperlooper_command::one_shot:   return "oneshot";
        case sooperlooper_command::trigger:    return "trigger";
        case sooperlooper_command::substitute: return "substitute";
        case sooperlooper_command::pause:      return "pause";
        case sooperlooper_command::solo:       return "solo";
        case sooperlooper_command::mute_on:    return "mute_on";
        case sooperlooper_command::mute_off:   return "mute_off";
    }
    return "";                                  // out-of-range / unknown
}

/* -------------------------------------------------------------------------
 *  loop_control  (sent/received via /sl/<index>/set and /sl/<index>/get)
 *
 *  The `kind` column is the *semantic* payload kind used by the receiver;
 *  it is independent of the wire-level range enforced by `is_in_range()`.
 *  e.g. `rate` is semantically a `ratio` toggle but the engine accepts
 *  0.25..4 on the wire.
 * ------------------------------------------------------------------------- */

namespace
{

struct loop_control_entry
{
    loop_control          id;
    const char *          name;
    payload_kind          kind;
    float                 lo;
    float                 hi;
    bound_mode            lo_mode;
    bound_mode            hi_mode;
};

constexpr bound_mode incl = bound_mode::inclusive;
constexpr bound_mode excl = bound_mode::exclusive;

constexpr std::array<loop_control_entry, 51> loop_control_table = {{
    { loop_control::rec_thresh,          "rec_thresh",          payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::feedback,            "feedback",            payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::use_feedback_play,   "use_feedback_play",   payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::dry,                 "dry",                 payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::wet,                 "wet",                 payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::input_gain,          "input_gain",          payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::use_safety_feedback, "use_safety_feedback", payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::rate,                "rate",                payload_kind::ratio,           0.25f,  4.0f,     incl, incl },
    { loop_control::use_rate,            "use_rate",            payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::stretch_ratio,       "stretch_ratio",       payload_kind::ratio,           0.5f,   4.0f,     incl, incl },
    { loop_control::pitch_shift,         "pitch_shift",         payload_kind::ratio,         -12.0f,  12.0f,     incl, incl },
    { loop_control::tempo_stretch,       "tempo_stretch",       payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::round_integer_tempo, "round_integer_tempo", payload_kind::boolean,         0.0f,   1.0f,     incl, incl },
    { loop_control::quantize,            "quantize",            payload_kind::indexed_quantize,0.0f,   1.0f,     incl, incl },
    { loop_control::round,               "round",               payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::sync,                "sync",                payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::playback_sync,       "playback_sync",       payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::relative_sync,       "relative_sync",       payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::mute_quantized,      "mute_quantized",      payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::overdub_quantized,   "overdub_quantized",   payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::replace_quantized,   "replace_quantized",   payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::redo_is_tap,         "redo_is_tap",         payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::jack_timebase_master,"jack_timebase_master",payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::fade_samples,        "fade_samples",        payload_kind::seconds,         0.0f,   4096.0f,  incl, incl },
    { loop_control::input_latency,       "input_latency",       payload_kind::seconds,         0.0f,   32768.0f, incl, incl },
    { loop_control::output_latency,      "output_latency",      payload_kind::seconds,         0.0f,   32768.0f, incl, incl },
    { loop_control::trigger_latency,     "trigger_latency",     payload_kind::seconds,         0.0f,   32768.0f, incl, incl },
    { loop_control::autoset_latency,     "autoset_latency",     payload_kind::seconds,         0.0f,   32768.0f, incl, incl },
    { loop_control::discrete_prefader,   "discrete_prefader",   payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::use_common_ins,      "use_common_ins",      payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::use_common_outs,     "use_common_outs",     payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::pan_1,               "pan_1",               payload_kind::ratio,          -1.0f,   1.0f,     incl, incl },
    { loop_control::pan_2,               "pan_2",               payload_kind::ratio,          -1.0f,   1.0f,     incl, incl },
    { loop_control::pan_3,               "pan_3",               payload_kind::ratio,          -1.0f,   1.0f,     incl, incl },
    { loop_control::pan_4,               "pan_4",               payload_kind::ratio,          -1.0f,   1.0f,     incl, incl },
    { loop_control::scratch_pos,         "scratch_pos",         payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::delay_trigger,       "delay_trigger",       payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::waiting,             "waiting",             payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::state,               "state",               payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::next_state,          "next_state",          payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::loop_len,            "loop_len",            payload_kind::seconds,         0.0f,   32768.0f, incl, incl },
    { loop_control::loop_pos,            "loop_pos",            payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::cycle_len,           "cycle_len",           payload_kind::seconds,         0.0f,   32768.0f, incl, incl },
    { loop_control::free_time,           "free_time",           payload_kind::seconds,         0.0f,   32768.0f, incl, incl },
    { loop_control::total_time,          "total_time",          payload_kind::seconds,         0.0f,   32768.0f, incl, incl },
    { loop_control::rate_output,         "rate_output",         payload_kind::seconds,         0.0f,   192000.0f,incl, incl },
    { loop_control::has_discrete_io,     "has_discrete_io",     payload_kind::boolean,         0.0f,   1.0f,     incl, incl },
    { loop_control::channel_count,       "channel_count",       payload_kind::seconds,         1.0f,   16.0f,    incl, incl },
    { loop_control::in_peak_meter,       "in_peak_meter",       payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::out_peak_meter,      "out_peak_meter",      payload_kind::ratio,           0.0f,   1.0f,     incl, incl },
    { loop_control::is_soloed,           "is_soloed",           payload_kind::ratio,           0.0f,   1.0f,     incl, incl }
}};

}               // anonymous namespace

const char * to_string (loop_control control)
{
    for (const auto & e : loop_control_table)
    {
        if (e.id == control)
            return e.name;
    }
    return "";
}

payload_kind metadata_for (loop_control control)
{
    for (const auto & e : loop_control_table)
    {
        if (e.id == control)
            return e.kind;
    }
    return payload_kind::unconstrained;
}

bool is_in_range (loop_control control, float value)
{
    if (! std::isfinite(value))
        return false;

    for (const auto & e : loop_control_table)
    {
        if (e.id == control)
        {
            bool lo_ok = (e.lo_mode == bound_mode::inclusive)
                ? (value >= e.lo) : (value > e.lo);
            bool hi_ok = (e.hi_mode == bound_mode::inclusive)
                ? (value <= e.hi) : (value < e.hi);
            return lo_ok && hi_ok;
        }
    }
    return false;                               // unknown control
}

bool try_parse (const std::string & name, loop_control & control)
{
    for (const auto & e : loop_control_table)
    {
        if (name == e.name)
        {
            control = e.id;
            return true;
        }
    }
    return false;                               // unknown string
}

/* -------------------------------------------------------------------------
 *  global_control  (sent/received via /set and /get at the engine root)
 * ------------------------------------------------------------------------- */

namespace
{

struct global_control_entry
{
    global_control        id;
    const char *          name;
    payload_kind          kind;
    float                 lo;
    float                 hi;
    bound_mode            lo_mode;
    bound_mode            hi_mode;
};

constexpr std::array<global_control_entry, 17> global_control_table = {{
    { global_control::tempo,                       "tempo",                       payload_kind::tempo_bpm,         0.0f,   1000.0f,  incl, incl },
    { global_control::eighth_per_cycle,            "eighth_per_cycle",            payload_kind::eighths_per_cycle, 0.0f,   2048.0f,  incl, incl },
    { global_control::sync_source,                 "sync_source",                 payload_kind::sync_source_index,-3.0f,  0.0f,     incl, incl },
    { global_control::tap_tempo,                   "tap_tempo",                   payload_kind::tempo_bpm,         0.0f,   1000.0f,  incl, incl },
    { global_control::save_loop,                   "save_loop",                   payload_kind::boolean,           0.0f,   1.0f,     incl, incl },
    { global_control::auto_disable_latency,        "auto_disable_latency",        payload_kind::boolean,           0.0f,   1.0f,     incl, incl },
    { global_control::select_next_loop,            "select_next_loop",            payload_kind::boolean,           0.0f,   1.0f,     incl, incl },
    { global_control::select_prev_loop,            "select_prev_loop",            payload_kind::boolean,           0.0f,   1.0f,     incl, incl },
    { global_control::select_all_loops,            "select_all_loops",            payload_kind::boolean,           0.0f,   1.0f,     incl, incl },
    { global_control::selected_loop_num,           "selected_loop_num",           payload_kind::integer_index,    -1.0f,  5.0f,     excl, excl },
    { global_control::output_midi_clock,           "output_midi_clock",           payload_kind::boolean,           0.0f,   1.0f,     incl, incl },
    { global_control::smart_eighths,               "smart_eighths",               payload_kind::boolean,           0.0f,   1.0f,     incl, incl },
    { global_control::use_midi_start,              "use_midi_start",              payload_kind::boolean,           0.0f,   1.0f,     incl, incl },
    { global_control::use_midi_stop,               "use_midi_stop",               payload_kind::boolean,           0.0f,   1.0f,     incl, incl },
    { global_control::send_midi_start_on_trigger,  "send_midi_start_on_trigger",  payload_kind::boolean,           0.0f,   1.0f,     incl, incl },
    { global_control::global_cycle_len,            "global_cycle_len",            payload_kind::seconds,           0.0f,   32768.0f, incl, incl },
    { global_control::global_cycle_pos,            "global_cycle_pos",            payload_kind::ratio,             0.0f,   32768.0f, incl, incl }
}};

}               // anonymous namespace

const char * to_string (global_control control)
{
    for (const auto & e : global_control_table)
    {
        if (e.id == control)
            return e.name;
    }
    return "";
}

payload_kind metadata_for (global_control control)
{
    for (const auto & e : global_control_table)
    {
        if (e.id == control)
            return e.kind;
    }
    return payload_kind::unconstrained;
}

bool is_in_range (global_control control, float value)
{
    if (! std::isfinite(value))
        return false;

    for (const auto & e : global_control_table)
    {
        if (e.id == control)
        {
            bool lo_ok = (e.lo_mode == bound_mode::inclusive)
                ? (value >= e.lo) : (value > e.lo);
            bool hi_ok = (e.hi_mode == bound_mode::inclusive)
                ? (value <= e.hi) : (value < e.hi);
            return lo_ok && hi_ok;
        }
    }
    return false;                               // unknown control
}

bool try_parse (const std::string & name, global_control & control)
{
    for (const auto & e : global_control_table)
    {
        if (name == e.name)
        {
            control = e.id;
            return true;
        }
    }
    return false;
}

/* -------------------------------------------------------------------------
 *  Engine state integer parsing
 *
 *  Source of truth: SooperLooper `src/loop_control` (LoopStateValues).
 *  Per `doc/sooperlooper/OSC-CONTROL-AND-FEEDBACK.md`, public state
 *  values are -1 and 0..14 and 20.  Unknown future values are preserved
 *  in `result.raw` and reported with `known == false`.
 * ------------------------------------------------------------------------- */

namespace
{

struct state_entry
{
    int           raw;
    const char *  label;
};

constexpr std::array<state_entry, 18> state_table = {{
    { -1, "unknown"      },
    {  0, "off"          },
    {  1, "wait_start"   },
    {  2, "recording"    },
    {  3, "wait_stop"    },
    {  4, "playing"      },
    {  5, "overdubbing"  },
    {  6, "multiplying"  },
    {  7, "inserting"    },
    {  8, "replacing"    },
    {  9, "delay"        },
    { 10, "muted"        },
    { 11, "scratching"   },
    { 12, "one_shot"     },
    { 13, "substituting" },
    { 14, "paused"       },
    { 15, "full_cycle"   },
    { 20, "off_muted"    }
}};

}               // anonymous namespace

void parse_state_int (int raw, state_parse_result & result)
{
    result.raw = raw;
    result.known = false;
    result.label = nullptr;

    for (const auto & e : state_table)
    {
        if (e.raw == raw)
        {
            result.known = true;
            result.label = e.label;
            return;
        }
    }
    /* unknown raw value: known remains false; raw is preserved verbatim. */
}

}           // namespace seq66

/*
 * sooperlooper_protocol.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */