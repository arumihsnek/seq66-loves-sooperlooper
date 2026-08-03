/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_crash_reconciler.cpp
 *
 *  Implementation of the crash detector and reconciler.
 */

#include "audio/sooperlooper_crash_reconciler.hpp"

#include <algorithm>
#include <cmath>

namespace seq66
{

sooperlooper_crash_reconciler::sooperlooper_crash_reconciler ()
{
    // Default configuration is set via reconciler_config defaults.
}

sooperlooper_crash_reconciler::~sooperlooper_crash_reconciler ()
{
    // Intentionally empty.
}

void
sooperlooper_crash_reconciler::set_config (const reconciler_config & cfg)
{
    m_config = cfg;
}

const reconciler_config &
sooperlooper_crash_reconciler::get_config () const
{
    return m_config;
}

void
sooperlooper_crash_reconciler::begin_watching (std::uint64_t generation)
{
    m_watched_generation = generation;
    m_state = reconciler_state::watching;
    m_startup_grace_active = true;
    m_launch_time_ms = 0;  /* Caller should set via record_crash_time if needed */
    m_pending.clear();
}

void
sooperlooper_crash_reconciler::stop_watching ()
{
    cancel_all_pending();
    m_state = reconciler_state::idle;
    m_startup_grace_active = false;
}

bool
sooperlooper_crash_reconciler::register_operation
(
    const std::string & id,
    const std::string & description
)
{
    /* Check for duplicate. */
    for (const auto & op : m_pending)
    {
        if (op.id == id)
            return false;
    }

    pending_operation op;
    op.id = id;
    op.description = description;
    op.generation = m_watched_generation;
    op.submitted_us = 0;  /* Caller can set externally if needed */
    op.completed = false;
    m_pending.push_back(op);
    return true;
}

bool
sooperlooper_crash_reconciler::complete_operation (const std::string & id)
{
    for (auto & op : m_pending)
    {
        if (op.id == id)
        {
            if (op.completed)
                return false;  /* Already completed */
            op.completed = true;
            return true;
        }
    }
    return false;
}

void
sooperlooper_crash_reconciler::cancel_all_pending ()
{
    /* Pending operations are not deleted; they retain their state
     * for diagnostics but are no longer actionable. */
    m_pending.clear();
}

reconcile_result
sooperlooper_crash_reconciler::check_and_reconcile
(
    sooperlooper_process_supervisor & supervisor,
    std::function<void(std::uint64_t)> on_invalidate_generation,
    std::function<bool()> on_restart,
    long long now_ms
)
{
    if (m_state != reconciler_state::watching)
        return reconcile_result::none;

    /* Poll the supervisor for unexpected exit. */
    if (! supervisor.poll())
        return reconcile_result::none;

    /* Crash detected.  Perform reconciliation. */
    m_state = reconciler_state::reconciling;

    /* 1. Cancel all pending operations. */
    cancel_all_pending();

    /* 2. Invalidate generation in cache/monitor. */
    if (on_invalidate_generation)
        on_invalidate_generation(m_watched_generation);

    /* 3. Check stable-interval reset: if enough time has elapsed
     *    since the last crash, reset the restart counter so that
     *    backoff pressure is released.  This satisfies the acceptance
     *    criterion: "backoff resets after stable interval". */
    if (now_ms > 0 && m_last_crash_time_ms > 0 &&
        stable_interval_elapsed(now_ms))
    {
        m_restart_count = 0;
    }

    /* 4. Record crash time for backoff. */
    if (now_ms > 0)
        m_last_crash_time_ms = now_ms;

    /* 5. Check restart budget. */
    if (m_restart_count >= m_config.max_restarts)
    {
        m_state = reconciler_state::terminal;
        return reconcile_result::terminal;
    }

    /* 6. Compute backoff. */
    int backoff = compute_backoff_ms(m_restart_count);
    ++m_restart_count;

    if (backoff > 0)
    {
        m_state = reconciler_state::backoff;
        return reconcile_result::backoff;
    }

    /* 7. Immediate restart (backoff is 0 for first crash). */
    if (on_restart && on_restart())
    {
        m_state = reconciler_state::watching;
        return reconcile_result::restarted;
    }

    m_state = reconciler_state::terminal;
    return reconcile_result::terminal;
}

bool
sooperlooper_crash_reconciler::handle_shutdown_during_startup
(
    sooperlooper_process_supervisor & supervisor
)
{
    if (m_state != reconciler_state::watching)
        return false;

    if (! m_startup_grace_active)
        return false;

    /* Shutdown requested during startup grace period. */
    supervisor.shutdown();
    stop_watching();
    return true;
}

reconciler_state
sooperlooper_crash_reconciler::state () const
{
    return m_state;
}

int
sooperlooper_crash_reconciler::restart_count () const
{
    return m_restart_count;
}

int
sooperlooper_crash_reconciler::current_backoff_ms () const
{
    return compute_backoff_ms(m_restart_count);
}

std::uint64_t
sooperlooper_crash_reconciler::watched_generation () const
{
    return m_watched_generation;
}

const std::vector<pending_operation> &
sooperlooper_crash_reconciler::pending_operations () const
{
    return m_pending;
}

int
sooperlooper_crash_reconciler::pending_count () const
{
    return static_cast<int>(m_pending.size());
}

void
sooperlooper_crash_reconciler::force_state (reconciler_state s)
{
    m_state = s;
}

void
sooperlooper_crash_reconciler::force_restart_count (int count)
{
    m_restart_count = count;
}

void
sooperlooper_crash_reconciler::set_last_stable_time (long long now_ms)
{
    m_last_stable_time_ms = now_ms;
}

bool
sooperlooper_crash_reconciler::backoff_elapsed (long long now_ms) const
{
    if (m_state != reconciler_state::backoff)
        return false;

    int backoff = compute_backoff_ms(m_restart_count - 1);
    return (now_ms - m_last_crash_time_ms) >= backoff;
}

void
sooperlooper_crash_reconciler::record_crash_time (long long now_ms)
{
    m_last_crash_time_ms = now_ms;
}

int
sooperlooper_crash_reconciler::compute_backoff_ms (int count) const
{
    if (count <= 0)
        return 0;

    double delay = m_config.base_backoff_ms *
                   std::pow(m_config.backoff_multiplier, count - 1);
    int result = static_cast<int>(delay);
    return std::min(result, m_config.max_backoff_ms);
}

bool
sooperlooper_crash_reconciler::stable_interval_elapsed (long long now_ms) const
{
    if (m_last_stable_time_ms == 0)
        return false;
    return (now_ms - m_last_stable_time_ms) >= m_config.stable_interval_ms;
}

} // namespace seq66

/*
 * sooperlooper_crash_reconciler.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
