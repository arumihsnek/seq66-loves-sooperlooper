/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_engine_launcher.cpp
 *
 *  Engine launch orchestration implementation.
 */

#include "audio/sooperlooper_engine_launcher.hpp"
#include "audio/sooperlooper_observed_state.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <functional>

namespace seq66
{

/* ------------------------------------------------------------------ */
/*  Naming policy                                                      */
/* ------------------------------------------------------------------ */

/*
 *  Simple deterministic hash for naming.  Not cryptographic — just
 *  sufficient for collision-resistant deterministic names.
 */
static std::uint64_t
simple_hash (const std::string & s)
{
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (unsigned char c : s)
    {
        h ^= static_cast<std::uint64_t>(c);
        h *= 0x100000001b3ULL;
    }
    return h;
}

int
engine_naming_policy::compute_osc_port (std::uint64_t generation) const
{
    std::string key = instance_id + ":" + std::to_string(generation);
    std::uint64_t h = simple_hash(key);
    return static_cast<int>(
        osc_port_base + (h % static_cast<std::uint64_t>(osc_port_range))
    );
}

std::string
engine_naming_policy::compute_jack_client_name (
    std::uint64_t generation) const
{
    std::string key = instance_id + ":" + std::to_string(generation);
    std::uint64_t h = simple_hash(key);

    /* Short hex prefix from hash for readability */
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%s-%08x-g%lu",
        jack_client_prefix.empty() ? "seq66-sl" : jack_client_prefix.c_str(),
        static_cast<unsigned>(h & 0xFFFFFFFF),
        static_cast<unsigned long>(generation));

    std::string name(buf);

    /* JACK client name limit is typically 63 characters */
    if (name.size() > 63)
        name = name.substr(0, 63);

    return name;
}

/* ------------------------------------------------------------------ */
/*  Construction and configuration                                     */
/* ------------------------------------------------------------------ */

sooperlooper_engine_launcher::sooperlooper_engine_launcher ()
    : m_adapter(nullptr)
{
}

sooperlooper_engine_launcher::sooperlooper_engine_launcher (
    std::shared_ptr<engine_launcher_adapter> adapter,
    std::shared_ptr<process_adapter> proc_adapter
)
    : m_adapter(std::move(adapter))
    , m_supervisor(std::move(proc_adapter))
{
}

sooperlooper_engine_launcher::~sooperlooper_engine_launcher ()
{
    if (m_state == launcher_state::running ||
        m_state == launcher_state::launching)
    {
        shutdown();
    }
    release_names();
}

void
sooperlooper_engine_launcher::set_naming_policy (
    const engine_naming_policy & policy)
{
    if (m_state == launcher_state::idle)
        m_naming_policy = policy;
}

const engine_naming_policy &
sooperlooper_engine_launcher::naming_policy () const
{
    return m_naming_policy;
}

void
sooperlooper_engine_launcher::set_supervisor_config (
    const supervisor_config & cfg)
{
    if (m_state == launcher_state::idle)
        m_supervisor.set_config(cfg);
}

void
sooperlooper_engine_launcher::set_engine_monitor (
    sooperlooper_engine_monitor * monitor)
{
    m_monitor = monitor;
}

void
sooperlooper_engine_launcher::set_observed_cache (
    sooperlooper_observed_cache * cache)
{
    m_cache = cache;
}

/* ------------------------------------------------------------------ */
/*  Launch sequence                                                    */
/* ------------------------------------------------------------------ */

launcher_result
sooperlooper_engine_launcher::launch ()
{
    if (m_state == launcher_state::running ||
        m_state == launcher_state::launching)
    {
        m_last_error = "engine already running";
        return launcher_result::supervisor_failed;
    }

    m_last_error.clear();

    /* Step 1: Probe backend */
    m_state = launcher_state::probing;

    if (! m_adapter)
    {
        m_state = launcher_state::failed;
        m_last_error = "no launcher adapter configured";
        return launcher_result::probe_error;
    }

    m_last_probe = m_adapter->probe_backend();

    if (m_last_probe.is_unavailable())
    {
        m_state = launcher_state::idle;
        m_last_error = "backend unavailable: " + m_last_probe.error;
        return launcher_result::backend_unavailable;
    }

    if (m_last_probe.has_error())
    {
        m_state = launcher_state::failed;
        m_last_error = "probe error: " + m_last_probe.error;
        return launcher_result::probe_error;
    }

    /* Step 2: Compute prospective generation */
    std::uint64_t prospective_gen = compute_prospective_generation();

    /* Step 3: Compute and reserve deterministic names */
    m_state = launcher_state::naming;

    if (! reserve_names(prospective_gen))
    {
        m_state = launcher_state::failed;
        m_last_error = "naming conflict or port unavailable";
        return launcher_result::naming_conflict;
    }

    /* Step 4: Configure supervisor with names and arguments */
    m_state = launcher_state::launching;
    m_generation = prospective_gen;

    /* Add OSC port and JACK client name to supervisor args */
    supervisor_config cfg = m_supervisor.get_config();
    /* Clear previous dynamic args, keep base args */
    std::vector<std::string> base_args;
    if (! cfg.args.empty())
        base_args = cfg.args;

    cfg.args = base_args;
    cfg.args.push_back("--osc-port");
    cfg.args.push_back(std::to_string(m_reserved_port));
    cfg.args.push_back("--jack-name");
    cfg.args.push_back(m_jack_client_name);
    cfg.args.push_back("--jack-server");
    cfg.args.push_back("");  /* empty = default server */
    m_supervisor.set_config(cfg);

    /* Step 5: Launch via supervisor */
    auto launch_result = m_supervisor.launch();

    if (launch_result != seq66::launch_result::success)
    {
        release_names();
        m_state = launcher_state::failed;
        m_last_error = "supervisor launch failed";
        return launcher_result::supervisor_failed;
    }

    /* Step 6: Publish generation to observed cache */
    publish_generation(m_generation);

    /* Step 7: Transition to running */
    m_state = launcher_state::running;

    return launcher_result::success;
}

/* ------------------------------------------------------------------ */
/*  Shutdown                                                           */
/* ------------------------------------------------------------------ */

bool
sooperlooper_engine_launcher::shutdown ()
{
    if (m_state == launcher_state::idle)
        return true;

    m_state = launcher_state::stopping;

    /* Delegate to supervisor */
    m_supervisor.shutdown();

    /* Release reserved names */
    release_names();

    m_state = launcher_state::idle;
    m_generation = 0;
    return true;
}

/* ------------------------------------------------------------------ */
/*  State queries                                                      */
/* ------------------------------------------------------------------ */

launcher_state
sooperlooper_engine_launcher::state () const
{
    return m_state;
}

std::uint64_t
sooperlooper_engine_launcher::generation () const
{
    return m_generation;
}

const backend_probe_result &
sooperlooper_engine_launcher::last_probe () const
{
    return m_last_probe;
}

const std::string &
sooperlooper_engine_launcher::last_error () const
{
    return m_last_error;
}

int
sooperlooper_engine_launcher::current_osc_port () const
{
    return m_reserved_port;
}

const std::string &
sooperlooper_engine_launcher::current_jack_client_name () const
{
    return m_jack_client_name;
}

bool
sooperlooper_engine_launcher::engine_alive () const
{
    return m_supervisor.child_alive();
}

void
sooperlooper_engine_launcher::force_state (launcher_state s)
{
    m_state = s;
}

const sooperlooper_process_supervisor &
sooperlooper_engine_launcher::supervisor () const
{
    return m_supervisor;
}

/* ------------------------------------------------------------------ */
/*  Private helpers                                                    */
/* ------------------------------------------------------------------ */

std::uint64_t
sooperlooper_engine_launcher::compute_prospective_generation () const
{
    /* The supervisor tracks its own generation.  The launcher's
     * prospective generation is supervisor.generation() + 1 because
     * the supervisor increments on launch(). */
    return m_supervisor.generation() + 1;
}

bool
sooperlooper_engine_launcher::reserve_names (std::uint64_t gen)
{
    m_reserved_port = m_naming_policy.compute_osc_port(gen);
    m_jack_client_name = m_naming_policy.compute_jack_client_name(gen);

    /* Verify port availability */
    if (m_adapter && ! m_adapter->is_port_available(m_reserved_port))
    {
        m_last_error = "OSC port " + std::to_string(m_reserved_port) +
                       " is not available";
        return false;
    }

    if (m_adapter)
        m_adapter->reserve_port(m_reserved_port);

    return true;
}

void
sooperlooper_engine_launcher::release_names ()
{
    if (m_reserved_port > 0 && m_adapter)
    {
        m_adapter->release_port(m_reserved_port);
        m_reserved_port = 0;
    }
    m_jack_client_name.clear();
}

void
sooperlooper_engine_launcher::publish_generation (std::uint64_t gen)
{
    if (m_cache)
        m_cache->set_generation(gen);

    if (m_monitor)
    {
        m_monitor->reset();
        m_monitor->force_state(engine_state::starting);
    }
}

} // namespace seq66

/*
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
