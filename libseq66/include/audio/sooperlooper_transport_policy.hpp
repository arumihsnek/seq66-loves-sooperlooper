#if ! defined SEQ66_SOOPERLOOPER_TRANSPORT_POLICY_HPP
#define SEQ66_SOOPERLOOPER_TRANSPORT_POLICY_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_transport_policy.hpp
 *
 *  Transport policy for managed SooperLooper engine.
 *
 *  Seq66 owns transport decisions (start/stop).  SooperLooper follows
 *  the transport policy: sync source, quantized start, and loop
 *  synchronisation.
 *
 *  Transport modes:
 *  - internal: engine uses its own clock;
 *  - JACK: engine follows JACK transport;
 *  - MIDI: engine follows MIDI start/stop/clock;
 *  - none: engine has no external sync (loops free-run).
 *
 *  The policy does NOT own OSC or the engine process.  It derives the
 *  required control settings and lets the caller apply them.
 */

#include <cstdint>
#include <string>

namespace seq66
{

/**
 *  Transport sync source.
 *
 *  Maps to the global_control::sync_source values documented in
 *  OSC-CONTROL-AND-FEEDBACK.md.
 */
enum class transport_sync_source
{
    none,           /**< No external sync (loop index 0..N).  */
    jack,           /**< JACK transport sync (-1).            */
    midi,           /**< MIDI start/stop/clock (-2).          */
    internal        /**< Engine internal clock (-3).          */
};

/**
 *  Transport state as observed from Seq66's perspective.
 */
enum class transport_state
{
    stopped,
    starting,
    playing,
    stopping
};

/**
 *  Transport policy configuration.
 *
 *  Each field corresponds to a global SooperLooper control that Seq66
 *  sets based on its own transport policy.
 */
struct transport_config
{
    /** Sync source. */
    transport_sync_source sync_source{transport_sync_source::jack};

    /** Whether MIDI start should be sent. */
    bool use_midi_start{false};

    /** Whether MIDI stop should be sent. */
    bool use_midi_stop{false};

    /** Whether to send MIDI clock output. */
    bool output_midi_clock{false};

    /** Whether to send start on trigger. */
    bool send_midi_start_on_trigger{false};
};

/**
 *  Transport policy.
 *
 *  Derives the OSC control values needed to apply Seq66's transport
 *  policy to SooperLooper.  Does not perform OSC I/O itself.
 */
class sooperlooper_transport_policy
{
public:
    sooperlooper_transport_policy () = default;
    ~sooperlooper_transport_policy () = default;

    /**
     *  Get the current transport configuration.
     */
    const transport_config & config () const;

    /**
     *  Set the transport configuration.
     */
    void set_config (const transport_config & cfg);

    /**
     *  Get the sync source value for the global_control.
     *
     *  Returns the integer value to send as sync_source:
     *  -3 = internal, -2 = MIDI, -1 = JACK, 0+ = none/loop.
     */
    int sync_source_value () const;

    /**
     *  Set the sync source from a symbolic name.
     *
     *  Returns true if the name was recognised.
     */
    bool set_sync_source (const std::string & name);

    /**
     *  Set the sync source from the raw integer value.
     *
     *  Recognises -3 (internal), -2 (MIDI), -1 (JACK), 0+ (none).
     */
    void set_sync_source_from_value (int value);

    /**
     *  Get the current transport state.
     */
    transport_state state () const;

    /**
     *  Request a transport state transition.
     *
     *  Returns true if the transition is valid.
     */
    bool request_start ();
    bool request_stop ();

    /**
     *  Check if JACK sync is configured.
     */
    bool is_jack_sync () const;

    /**
     *  Check if MIDI sync is configured.
     */
    bool is_midi_sync () const;

    /**
     *  Check if no sync (free-running).
     */
    bool is_no_sync () const;

private:
    transport_config m_config;
    transport_state m_state{transport_state::stopped};
};

/**
 *  Returns a human-readable string for a transport_sync_source.
 */
const char * to_string (transport_sync_source src);

/**
 *  Returns a human-readable string for a transport_state.
 */
const char * to_string (transport_state state);

/**
 *  Parses a sync source string: "none", "jack", "midi", "internal".
 */
bool try_parse (const std::string & name, transport_sync_source & src);

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_TRANSPORT_POLICY_HPP

/*
 * sooperlooper_transport_policy.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
