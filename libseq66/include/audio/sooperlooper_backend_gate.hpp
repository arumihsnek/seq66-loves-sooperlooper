#if ! defined SEQ66_SOOPERLOOPER_BACKEND_GATE_HPP
#define SEQ66_SOOPERLOOPER_BACKEND_GATE_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_backend_gate.hpp
 *
 *  Backend gate for audio operations.
 *
 *  Wraps the backend probe to provide a gate that controls whether
 *  audio operations are allowed.  The gate is:
 *  - visible but disabled in ALSA-only mode
 *  - enabled when native JACK or PipeWire-JACK is detected
 *  - enforced from UI, keyboard, MIDI, and headless paths
 *  - never silently starts JACK, PipeWire, or SooperLooper
 */

#include <string>
#include "audio/sooperlooper_backend_probe.hpp"

namespace seq66
{

/**
 *  Backend gate state.
 */
enum class backend_gate_state : int
{
    closed,             /**< Audio operations blocked (ALSA-only).       */
    open,               /**< Audio operations allowed (JACK/PipeWire).   */
    error               /**< Probe failed, gate state unknown.           */
};

/**
 *  Backend gate result with explanation.
 */
struct backend_gate_result
{
    backend_gate_state state{backend_gate_state::closed};
    std::string explanation;
    backend_capability capability{backend_capability::unknown};

    bool is_open () const { return state == backend_gate_state::open; }
    bool is_closed () const { return state == backend_gate_state::closed; }
    bool has_error () const { return state == backend_gate_state::error; }
};

/**
 *  Backend gate evaluator.
 *
 *  Evaluates the backend capability and produces a gate result.
 *  The gate is stateless — it reads the probe result and produces
 *  a decision without performing I/O.
 */
class sooperlooper_backend_gate
{
public:
    sooperlooper_backend_gate () = default;
    ~sooperlooper_backend_gate () = default;

    /**
     *  Evaluate the gate from a probe result.
     *
     *  \param probe  Backend probe result.
     *  \return Gate result with state and explanation.
     */
    backend_gate_result evaluate (const backend_probe_result & probe) const;

    /**
     *  Quick check — returns true if audio operations are allowed.
     */
    bool is_open (const backend_probe_result & probe) const;

    /**
     *  Get a human-readable explanation for the gate state.
     */
    std::string explanation (const backend_probe_result & probe) const;
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_BACKEND_GATE_HPP

/*
 * sooperlooper_backend_gate.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
