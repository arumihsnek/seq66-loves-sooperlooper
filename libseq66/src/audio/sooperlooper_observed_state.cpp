/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_observed_state.cpp
 *
 *  Observed-state cache implementation for SooperLooper receiver feedback.
 */

#include "audio/sooperlooper_observed_state.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace seq66
{

/*
 *  Helper: parse a string to float, returning 0.0f on failure.
 */
static float
parse_float (const std::string & s)
{
    char * end = nullptr;
    float v = std::strtof(s.c_str(), &end);
    return (end && end != s.c_str()) ? v : 0.0f;
}

/*
 *  Helper: parse a string to int, returning 0 on failure.
 */
static int
parse_int (const std::string & s)
{
    char * end = nullptr;
    int v = std::strtol(s.c_str(), &end, 10);
    return (end && end != s.c_str()) ? v : 0;
}

/*
 *  Helper: extract the loop index from an OSC path like "/sl/0/get".
 *  Returns -1 if the path does not match the expected pattern.
 */
static int
extract_loop_index (const std::string & path)
{
    // Expected pattern: "/sl/<index>/..." where index is a signed integer.
    const std::string prefix = "/sl/";
    if (path.size() < prefix.size())
        return -1;
    if (path.compare(0, prefix.size(), prefix) != 0)
        return -1;

    std::size_t start = prefix.size();
    std::size_t slash = path.find('/', start);
    if (slash == std::string::npos)
        return -1;

    std::string num_str = path.substr(start, slash - start);
    char * end = nullptr;
    long v = std::strtol(num_str.c_str(), &end, 10);
    if (end != num_str.c_str() + num_str.size())
        return -1;

    return static_cast<int>(v);
}

/*
 *  Helper: extract the control name from an OSC path.
 *  For "/sl/<index>/get" or "/sl/<index>/set" the control is in the args.
 *  For reply paths the control name is the second argument.
 *  This helper is not needed for apply() since the control comes from args.
 */
/*
 *  Determine whether a loop_control is a high-frequency telemetry field
 *  that may be coalesced (S6.5).
 */
/*
 *  Coalescing and transition-critical classification helpers.
 *  Reserved for M1-004 generation tracking and transition delivery.
 *  is_coalescible() and is_transition_critical() will be used when
 *  the ordered event channel is implemented.
 */

/*
 *  Apply a float value to an observed_field.
 */
static void
apply_field (observed_field<float> & field, const std::string & arg,
             long long timestamp_us)
{
    field.value = parse_float(arg);
    field.present = true;
    field.timestamp_us = timestamp_us;
}

/*
 *  Apply an int value to an observed_field.
 */
static void
apply_field (observed_field<int> & field, const std::string & arg,
             long long timestamp_us)
{
    field.value = parse_int(arg);
    field.present = true;
    field.timestamp_us = timestamp_us;
}

/*
 *  Try to apply a loop control from string arguments.
 *  Returns true if the control was recognised and applied.
 */
static bool
apply_loop_control (loop_observed_state & state, loop_control control,
                    const std::vector<std::string> & args,
                    long long timestamp_us)
{
    if (args.empty())
        return false;

    const std::string & val = args[0];

    switch (control)
    {
        case loop_control::state:
            apply_field(state.state, val, timestamp_us);
            return true;

        case loop_control::next_state:
            apply_field(state.next_state, val, timestamp_us);
            return true;

        case loop_control::waiting:
            apply_field(state.waiting, val, timestamp_us);
            return true;

        case loop_control::loop_len:
            apply_field(state.loop_len, val, timestamp_us);
            return true;

        case loop_control::loop_pos:
            apply_field(state.loop_pos, val, timestamp_us);
            return true;

        case loop_control::cycle_len:
            apply_field(state.cycle_len, val, timestamp_us);
            return true;

        case loop_control::rate_output:
            apply_field(state.rate_output, val, timestamp_us);
            return true;

        case loop_control::channel_count:
            apply_field(state.channel_count, val, timestamp_us);
            return true;

        case loop_control::is_soloed:
            apply_field(state.is_soloed, val, timestamp_us);
            return true;

        case loop_control::in_peak_meter:
            apply_field(state.in_peak_meter, val, timestamp_us);
            return true;

        case loop_control::out_peak_meter:
            apply_field(state.out_peak_meter, val, timestamp_us);
            return true;

        default:
            return false;
    }
}

/*
 *  Try to apply a global control from string arguments.
 *  Returns true if the control was recognised and applied.
 */
static bool
apply_global_control (global_observed_state & state, global_control control,
                      const std::vector<std::string> & args,
                      long long timestamp_us)
{
    if (args.empty())
        return false;

    const std::string & val = args[0];

    switch (control)
    {
        case global_control::tempo:
            apply_field(state.tempo, val, timestamp_us);
            return true;

        case global_control::eighth_per_cycle:
            apply_field(state.eighth_per_cycle, val, timestamp_us);
            return true;

        case global_control::sync_source:
            apply_field(state.sync_source, val, timestamp_us);
            return true;

        case global_control::global_cycle_len:
            apply_field(state.global_cycle_len, val, timestamp_us);
            return true;

        case global_control::global_cycle_pos:
            apply_field(state.global_cycle_pos, val, timestamp_us);
            return true;

        default:
            return false;
    }
}

/*
 *  Parse the control name from event args.
 *
 *  For SooperLooper reply callbacks the format is:
 *      i:loop_index  s:control  f:value
 *  or for global:
 *      s:control  f:value
 *
 *  The args vector contains the stringified versions of these.
 *  For per-loop events, args[0] is the loop index and args[1] is
 *  the control name.  For global events, args[0] is the control name.
 *
 *  This function determines whether the event is per-loop or global
 *  based on the path, and returns the control name and loop index.
 */
bool
sooperlooper_observed_cache::apply (const std::string & path,
                                    const std::string & /* types */,
                                    const std::vector<std::string> & args,
                                    long long timestamp_us)
{
    if (args.empty())
        return false;

    // Determine if this is a per-loop or global event based on path.
    int loop_index = extract_loop_index(path);

    std::lock_guard<std::mutex> lock(m_mutex);

    if (loop_index >= 0)
    {
        // Per-loop event: args[0] is the control name, args[1] is the value.
        if (args.size() < 2)
            return false;

        loop_control control;
        if (!try_parse(args[0], control))
            return false;   // Unknown control: reject safely.

        // Ensure the loop entry exists.
        auto & state = m_loops[loop_index];

        // Apply the value.  Coalescing is implicit: latest value wins.
        // Transition-critical fields (state, next_state, waiting) are
        // always applied in arrival order; the cache stores only the
        // latest value, but consumers that need every transition should
        // use the receiver's ordered dispatch path.
        if (apply_loop_control(state, control, {args[1]}, timestamp_us))
        {
            m_dirty = true;
            return true;
        }
        return false;
    }
    else
    {
        // Global event: args[0] is the control name, args[1] is the value.
        if (args.size() < 2)
            return false;

        global_control control;
        if (!try_parse(args[0], control))
            return false;   // Unknown control: reject safely.

        if (apply_global_control(m_global, control, {args[1]}, timestamp_us))
        {
            m_dirty = true;
            return true;
        }
        return false;
    }
}

sooperlooper_observed_cache::snapshot_data
sooperlooper_observed_cache::snapshot () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    snapshot_data snap;
    snap.loops = m_loops;
    snap.global = m_global;
    snap.dirty = m_dirty;
    return snap;
}

void
sooperlooper_observed_cache::clear ()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_loops.clear();
    m_global = global_observed_state{};
    m_dirty = false;
}

bool
sooperlooper_observed_cache::dirty () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dirty;
}

std::size_t
sooperlooper_observed_cache::loop_count () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_loops.size();
}

bool
sooperlooper_observed_cache::is_present (int loop_index,
                                          loop_control control) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_loops.find(loop_index);
    if (it == m_loops.end())
        return false;

    const auto & s = it->second;
    switch (control)
    {
        case loop_control::state:          return s.state.present;
        case loop_control::next_state:     return s.next_state.present;
        case loop_control::waiting:        return s.waiting.present;
        case loop_control::loop_len:       return s.loop_len.present;
        case loop_control::loop_pos:       return s.loop_pos.present;
        case loop_control::cycle_len:      return s.cycle_len.present;
        case loop_control::rate_output:    return s.rate_output.present;
        case loop_control::channel_count:  return s.channel_count.present;
        case loop_control::is_soloed:      return s.is_soloed.present;
        case loop_control::in_peak_meter:  return s.in_peak_meter.present;
        case loop_control::out_peak_meter: return s.out_peak_meter.present;
        default:                           return false;
    }
}

} // namespace seq66

/*
 * sooperlooper_observed_state.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
