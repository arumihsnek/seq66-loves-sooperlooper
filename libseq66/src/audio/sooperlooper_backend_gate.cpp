/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_backend_gate.cpp
 *
 *  Backend gate implementation.
 */

#include "audio/sooperlooper_backend_gate.hpp"

namespace seq66
{

backend_gate_result
sooperlooper_backend_gate::evaluate (const backend_probe_result & probe) const
{
    backend_gate_result result;
    result.capability = probe.capability;

    if (probe.is_usable())
    {
        result.state = backend_gate_state::open;
        result.explanation = "Audio backend available: " +
                             probe.implementation;
    }
    else if (probe.is_unavailable())
    {
        result.state = backend_gate_state::closed;
        result.explanation = "Audio backend unavailable: " + probe.error +
                             ". Native JACK or PipeWire-JACK required.";
    }
    else if (probe.has_error())
    {
        result.state = backend_gate_state::error;
        result.explanation = "Backend probe error: " + probe.error;
    }
    else
    {
        result.state = backend_gate_state::closed;
        result.explanation = "Audio backend state unknown.";
    }

    return result;
}

bool
sooperlooper_backend_gate::is_open (const backend_probe_result & probe) const
{
    return evaluate(probe).is_open();
}

std::string
sooperlooper_backend_gate::explanation (const backend_probe_result & probe) const
{
    return evaluate(probe).explanation;
}

} // namespace seq66

/*
 * sooperlooper_backend_gate.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
