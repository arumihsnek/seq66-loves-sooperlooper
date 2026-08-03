#if ! defined SEQ66_SOOPERLOOPER_READINESS_GATE_HPP
#define SEQ66_SOOPERLOOPER_READINESS_GATE_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_readiness_gate.hpp
 *
 *  Engine readiness gate for SooperLooper integration.
 *
 *  Combines evidence from: backend capability, process health, OSC
 *  ping/version, JACK topology, and routing into a single readiness
 *  determination.  The gate is deterministic and does not perform I/O.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "audio/sooperlooper_backend_probe.hpp"
#include "audio/sooperlooper_engine_monitor.hpp"
#include "audio/sooperlooper_engine_jack_discovery.hpp"

namespace seq66
{

/**
 *  Overall engine readiness state.
 */
enum class engine_readiness : int
{
    not_ready,          /**< Engine is not ready for operation.        */
    ready,              /**< All conditions met.                       */
    degraded,           /**< Partially operational with known issues.  */
    backend_unavailable /**< No supported audio backend.               */
};

/**
 *  Individual readiness condition.
 */
struct readiness_condition
{
    std::string name;           /**< Human-readable condition name.      */
    bool satisfied{false};      /**< True if condition is met.          */
    std::string detail;         /**< Explanation if not satisfied.       */
};

/**
 *  Complete readiness evaluation result.
 */
struct readiness_result
{
    engine_readiness readiness{engine_readiness::not_ready};
    std::vector<readiness_condition> conditions;
    std::string summary;
    std::uint64_t generation{0};
};

/**
 *  Readiness gate evaluator.
 *
 *  Combines all evidence sources into a single readiness determination.
 *  The evaluator is stateless — it reads external state and produces
 *  a result without performing I/O or mutating external state.
 *
 *  Readiness requires ALL of:
 *    1. Backend is usable (not backend_unavailable or probe_error).
 *    2. Process child is alive.
 *    3. OSC engine monitor is ready (ping replied, version compatible).
 *    4. JACK topology is complete (all expected connections exist).
 *
 *  Degraded state: some but not all conditions met (partial operation).
 */
class sooperlooper_readiness_gate
{
public:
    sooperlooper_readiness_gate () = default;
    ~sooperlooper_readiness_gate () = default;

    /**
     *  Evaluate readiness from the current state of all evidence sources.
     *
     *  \param backend        Backend probe result.
     *  \param child_alive    Whether the process child is alive.
     *  \param monitor_state  Current engine monitor state.
     *  \param topology       JACK topology check result.
     *  \param generation     Current engine generation.
     *
     *  \return Complete readiness evaluation.
     */
    readiness_result evaluate (
        const backend_probe_result & backend,
        bool child_alive,
        engine_state monitor_state,
        const jack_topology_result & topology,
        std::uint64_t generation) const;

    /**
     *  Quick readiness check — returns true only if fully ready.
     */
    bool is_ready (
        const backend_probe_result & backend,
        bool child_alive,
        engine_state monitor_state,
        const jack_topology_result & topology,
        std::uint64_t generation) const;

    /**
     *  Check if backend_unavailable (hard block).
     */
    bool is_backend_unavailable (
        const backend_probe_result & backend) const;

private:
    /** Check individual conditions. */
    readiness_condition check_backend (
        const backend_probe_result & backend) const;
    readiness_condition check_process (bool child_alive) const;
    readiness_condition check_osc (engine_state monitor_state) const;
    readiness_condition check_topology (
        const jack_topology_result & topology) const;
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_READINESS_GATE_HPP

/*
 * sooperlooper_readiness_gate.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
