/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_observed_state.hpp
 *
 *  Observed-state cache for SooperLooper receiver feedback.
 *
 *  This module provides a thread-safe store that separates desired from
 *  observed SooperLooper state.  Each observed field carries a freshness
 *  timestamp and a presence flag so that zero is distinguishable from
 *  absent/stale.  High-frequency meter and position fields support
 *  latest-value coalescing; state transitions and errors are never
 *  coalesced away.
 *
 *  Consumers read immutable snapshots without network calls.  The cache
 *  is independent of the receiver; events are fed externally via
 *  apply().  A clear() method is provided for engine-generation resets
 *  (M1-004 integration).
 */

#ifndef SEQ66_SOOPERLOOPER_OBSERVED_STATE_HPP
#define SEQ66_SOOPERLOOPER_OBSERVED_STATE_HPP

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "audio/sooperlooper_protocol.hpp"

namespace seq66
{

/**
 *  A single observed value with presence and freshness tracking.
 *
 *  The template parameter T is the value type (int, float, etc.).
 *  A field is "present" when it has been set at least once; a zero
 *  value with present==true is NOT the same as an absent field.
 */
template <typename T>
struct observed_field
{
    T value{};                      /**< The observed value.            */
    bool present{false};            /**< True once the field is set.    */
    long long timestamp_us{0};      /**< Microseconds since epoch.      */
};

/**
 *  Per-loop observed state.
 *
 *  Contains one observed_field for each control that appears in
 *  SooperLooper per-loop feedback (S7.2).  The field types match
 *  the payload semantics documented in the OSC contract.
 */
struct loop_observed_state
{
    /** Engine state integer (raw, preserved even if unknown). */
    observed_field<int> state;

    /** Next state integer. */
    observed_field<int> next_state;

    /** Waiting flag (0 or 1). */
    observed_field<int> waiting;

    /** Loop length in samples. */
    observed_field<float> loop_len;

    /** Current loop position in samples. */
    observed_field<float> loop_pos;

    /** Cycle length in samples. */
    observed_field<float> cycle_len;

    /** Actual output rate (1.0 = normal speed). */
    observed_field<float> rate_output;

    /** Channel count (1..16). */
    observed_field<int> channel_count;

    /** Solo flag (0 or 1). */
    observed_field<int> is_soloed;

    /** Input peak meter (>= 0, may exceed 1.0). */
    observed_field<float> in_peak_meter;

    /** Output peak meter (>= 0, may exceed 1.0). */
    observed_field<float> out_peak_meter;
};

/**
 *  Global observed state.
 *
 *  Contains one observed_field for each control that appears in
 *  SooperLooper global feedback.
 */
struct global_observed_state
{
    /** Current tempo in BPM. */
    observed_field<float> tempo;

    /** Eighth notes per cycle. */
    observed_field<float> eighth_per_cycle;

    /** Sync source index. */
    observed_field<int> sync_source;

    /** Global cycle length in samples. */
    observed_field<float> global_cycle_len;

    /** Global cycle position in samples. */
    observed_field<float> global_cycle_pos;
};

/**
 *  Observed-state cache.
 *
 *  Thread-safe store of SooperLooper feedback state.  The cache is
 *  fed externally via apply() and read via snapshot().  It does NOT
 *  own the receiver or perform any network operations.
 *
 *  Thread safety: a single mutex protects all mutations.  Snapshots
 *  are taken under the lock and returned as immutable copies.
 *  Multiple concurrent readers are safe; there is no single-writer
 *  restriction beyond the caller's own threading policy.
 *
 *  Coalescing: meter and position fields use latest-value replacement
 *  (no history).  State transitions and errors are always applied in
 *  arrival order and are never intentionally dropped.
 *
 *  Unknown controls: events with unrecognised control names are
 *  rejected before reaching the cache.  Unknown raw state integers
 *  are preserved inside the typed state field.
 */
class sooperlooper_observed_cache
{
public:
    /**
     *  Snapshot of the full observed state.
     *
     *  This is an immutable value returned by snapshot().  Consumers
     *  may read it freely without locking.
     */
    struct snapshot_data
    {
        /** Per-loop observed state keyed by loop index. */
        std::map<int, loop_observed_state> loops;

        /** Global observed state. */
        global_observed_state global;

        /** True if any state has been written since construction or last clear(). */
        bool dirty{false};
    };

private:
    /** Mutex protecting all internal state. */
    mutable std::mutex m_mutex;

    /** Per-loop observed state keyed by loop index. */
    std::map<int, loop_observed_state> m_loops;

    /** Global observed state. */
    global_observed_state m_global;

    /** True if any field has been updated since construction/clear(). */
    bool m_dirty{false};

public:
    sooperlooper_observed_cache () = default;
    ~sooperlooper_observed_cache () = default;

    // Disable copy and move (thread safety).
    sooperlooper_observed_cache (const sooperlooper_observed_cache &) = delete;
    sooperlooper_observed_cache & operator = (const sooperlooper_observed_cache &) = delete;

    /**
     *  Apply a received event to the cache.
     *
     *  The event must have a recognised control name.  Unknown
     *  controls are rejected (return false).  Meter and position
     *  fields use latest-value coalescing; all other fields are
     *  applied in arrival order.
     *
     *  \param path     The OSC path (e.g. "/sl/0/get").
     *  \param types    The OSC type tag string.
     *  \param args     The arguments as strings.
     *  \param timestamp_us  Timestamp in microseconds since epoch.
     *  \return true if the event was applied, false if rejected.
     */
    bool apply (const std::string & path, const std::string & types,
                const std::vector<std::string> & args, long long timestamp_us);

    /**
     *  Return an immutable snapshot of the full observed state.
     *
     *  The copy is taken under the lock; after return the caller
     *  may read it freely.
     */
    snapshot_data snapshot () const;

    /**
     *  Clear all observed state.
     *
     *  Used for engine-generation resets.  After clear(), all
     *  fields return to absent state and dirty becomes false.
     */
    void clear ();

    /**
     *  Return true if any field has been updated since construction
     *  or the last clear().
     */
    bool dirty () const;

    /**
     *  Return the number of known loop indexes in the cache.
     */
    std::size_t loop_count () const;

    /**
     *  Check whether a specific loop field is present.
     *
     *  \param loop_index   The loop index.
     *  \param control      The loop control to check.
     *  \return true if the field exists and has been set.
     */
    bool is_present (int loop_index, loop_control control) const;
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_OBSERVED_STATE_HPP

/*
 * sooperlooper_observed_state.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
