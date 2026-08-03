#if ! defined SEQ66_SOOPERLOOPER_TEMPO_POLICY_HPP
#define SEQ66_SOOPERLOOPER_TEMPO_POLICY_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_tempo_policy.hpp
 *
 *  Tempo policy for managed SooperLooper engine.
 *
 *  Three tempo modes:
 *
 *  - **free**: each loop runs independently at its own rate.
 *    No global tempo is applied; use_rate is false for all loops.
 *
 *  - **tape**: loop plays at recording speed (rate = 1.0).
 *    use_rate is true, rate is 1.0.  The loop synchronises to
 *    the engine clock but does not stretch.
 *
 *  - **elastic**: loop follows the global Seq66 tempo.
 *    use_rate is true, tempo_stretch is true.  The engine
 *    adjusts playback rate to match tempo changes.  Rate is
 *    computed as: target_tempo / recording_tempo.
 *
 *  The policy derives per-loop OSC control values and global tempo
 *  settings.  It does not perform OSC I/O itself.
 */

#include <cstdint>
#include <string>
#include <vector>

namespace seq66
{

/**
 *  Tempo mode.
 */
enum class tempo_mode
{
    free,       /**< Independent loops, no global tempo.    */
    tape,       /**< Play at recording speed (rate = 1.0).  */
    elastic     /**< Follow global tempo via rate/stretch.   */
};

/**
 *  Per-loop tempo configuration.
 *
 *  Derived from the tempo mode and loop's recording tempo.
 */
struct loop_tempo_config
{
    /** Runtime loop index (set by the mapper). */
    int runtime_index{-1};

    /** The tempo (BPM) at which this loop was recorded. */
    float recording_tempo{120.0f};

    /** Whether use_rate should be enabled. */
    bool use_rate{false};

    /** Whether tempo_stretch should be enabled. */
    bool tempo_stretch{false};

    /** Playback rate (0.25..4.0). */
    float rate{1.0f};

    /** Whether round_integer_tempo is enabled. */
    bool round_integer_tempo{false};

    /** Stretch ratio (0.5..4.0). */
    float stretch_ratio{1.0f};
};

/**
 *  Global tempo configuration.
 */
struct global_tempo_config
{
    /** Global tempo in BPM (0..1000). */
    float tempo{120.0f};

    /** Eighth notes per cycle. */
    float eighth_per_cycle{8.0f};
};

/**
 *  Tempo policy.
 *
 *  Derives the OSC control values needed to apply Seq66's tempo
 *  policy to SooperLooper.  Does not perform OSC I/O itself.
 */
class sooperlooper_tempo_policy
{
public:
    sooperlooper_tempo_policy () = default;
    ~sooperlooper_tempo_policy () = default;

    /**
     *  Get the current tempo mode.
     */
    tempo_mode mode () const;

    /**
     *  Set the tempo mode.
     */
    void set_mode (tempo_mode m);

    /**
     *  Set the tempo mode from a string ("free", "tape", "elastic").
     *
     *  Returns true if recognised.
     */
    bool set_mode (const std::string & name);

    /**
     *  Get the global tempo configuration.
     */
    const global_tempo_config & global_config () const;

    /**
     *  Set the global tempo BPM.
     */
    void set_global_tempo (float bpm);

    /**
     *  Get per-loop tempo configs.
     */
    const std::vector<loop_tempo_config> & loop_configs () const;

    /**
     *  Add a loop with a recording tempo.
     *
     *  Returns the loop index assigned.
     */
    int add_loop (float recording_tempo);

    /**
     *  Remove a loop by index.
     */
    bool remove_loop (int index);

    /**
     *  Clear all loops.
     */
    void clear_loops ();

    /**
     *  Compute the loop config for the current mode.
     *
     *  Call after set_mode() or set_global_tempo() to re-derive
     *  per-loop values.
     */
    void recompute ();

    /**
     *  Get the derived loop config at index.
     *
     *  Returns nullptr if index is out of range.
     */
    const loop_tempo_config * loop_config (int index) const;

    /**
     *  Compute the rate for a loop in elastic mode.
     *
     *  rate = global_tempo / recording_tempo, clamped to [0.25, 4.0].
     */
    float compute_rate (float global_tempo, float recording_tempo) const;

private:
    tempo_mode m_mode{tempo_mode::free};
    global_tempo_config m_global;
    std::vector<loop_tempo_config> m_loops;
};

/**
 *  Returns a human-readable string for a tempo_mode.
 */
const char * to_string (tempo_mode m);

/**
 *  Parses a tempo mode string: "free", "tape", "elastic".
 */
bool try_parse (const std::string & name, tempo_mode & m);

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_TEMPO_POLICY_HPP

/*
 * sooperlooper_tempo_policy.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
