/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_readiness_gate.cpp
 *
 *  Readiness gate evaluation implementation.
 */

#include "audio/sooperlooper_readiness_gate.hpp"

namespace seq66
{

readiness_result
sooperlooper_readiness_gate::evaluate (
    const backend_probe_result & backend,
    bool child_alive,
    engine_state monitor_state,
    const jack_topology_result & topology,
    std::uint64_t generation) const
{
    readiness_result result;
    result.generation = generation;

    /* Check each condition */
    result.conditions.push_back(check_backend(backend));
    result.conditions.push_back(check_process(child_alive));
    result.conditions.push_back(check_osc(monitor_state));
    result.conditions.push_back(check_topology(topology));

    /* Count satisfied conditions */
    int satisfied = 0;
    for (const auto & c : result.conditions)
    {
        if (c.satisfied)
            ++satisfied;
    }

    /* Determine overall readiness */
    bool all_ready = (satisfied == static_cast<int>(result.conditions.size()));

    if (backend.is_unavailable() || backend.has_error())
    {
        result.readiness = engine_readiness::backend_unavailable;
        result.summary = "backend unavailable";
    }
    else if (all_ready)
    {
        result.readiness = engine_readiness::ready;
        result.summary = "engine ready";
    }
    else if (satisfied > 0)
    {
        result.readiness = engine_readiness::degraded;
        result.summary = "degraded: " + std::to_string(satisfied) +
                         "/" + std::to_string(result.conditions.size()) +
                         " conditions met";
    }
    else
    {
        result.readiness = engine_readiness::not_ready;
        result.summary = "not ready: no conditions met";
    }

    return result;
}

bool
sooperlooper_readiness_gate::is_ready (
    const backend_probe_result & backend,
    bool child_alive,
    engine_state monitor_state,
    const jack_topology_result & topology,
    std::uint64_t generation) const
{
    return evaluate(backend, child_alive, monitor_state,
                    topology, generation).readiness ==
           engine_readiness::ready;
}

bool
sooperlooper_readiness_gate::is_backend_unavailable (
    const backend_probe_result & backend) const
{
    return backend.is_unavailable() || backend.has_error();
}

/* ------------------------------------------------------------------ */
/*  Individual condition checks                                         */
/* ------------------------------------------------------------------ */

readiness_condition
sooperlooper_readiness_gate::check_backend (
    const backend_probe_result & backend) const
{
    readiness_condition c;
    c.name = "backend";

    if (backend.is_unavailable())
    {
        c.satisfied = false;
        c.detail = "backend unavailable: " + backend.error;
    }
    else if (backend.has_error())
    {
        c.satisfied = false;
        c.detail = "probe error: " + backend.error;
    }
    else if (backend.is_usable())
    {
        c.satisfied = true;
        c.detail = "backend usable (" + backend.implementation + ")";
    }
    else
    {
        c.satisfied = false;
        c.detail = "backend state unknown";
    }

    return c;
}

readiness_condition
sooperlooper_readiness_gate::check_process (bool child_alive) const
{
    readiness_condition c;
    c.name = "process";
    c.satisfied = child_alive;
    c.detail = child_alive ? "child process alive" : "child process not alive";
    return c;
}

readiness_condition
sooperlooper_readiness_gate::check_osc (engine_state monitor_state) const
{
    readiness_condition c;
    c.name = "osc";

    if (monitor_state == engine_state::ready)
    {
        c.satisfied = true;
        c.detail = "engine monitor ready";
    }
    else
    {
        c.satisfied = false;

        switch (monitor_state)
        {
            case engine_state::disabled:
                c.detail = "engine disabled";
                break;
            case engine_state::starting:
                c.detail = "engine starting";
                break;
            case engine_state::reconciling:
                c.detail = "engine reconciling";
                break;
            case engine_state::stale:
                c.detail = "engine stale";
                break;
            case engine_state::engine_offline:
                c.detail = "engine offline";
                break;
            case engine_state::restarting:
                c.detail = "engine restarting";
                break;
            default:
                c.detail = "engine state unknown";
                break;
        }
    }

    return c;
}

readiness_condition
sooperlooper_readiness_gate::check_topology (
    const jack_topology_result & topology) const
{
    readiness_condition c;
    c.name = "topology";
    c.satisfied = topology.complete;

    if (topology.complete)
    {
        c.detail = "topology complete";
    }
    else
    {
        c.detail = "incomplete: " +
                   std::to_string(topology.missing_connections.size()) +
                   " missing connection(s)";
    }

    return c;
}

} // namespace seq66

/*
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
