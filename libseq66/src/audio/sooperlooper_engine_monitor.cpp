/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_engine_monitor.cpp
 *
 *  Engine lifecycle monitor implementation.
 */

#include "audio/sooperlooper_engine_monitor.hpp"

#include <algorithm>

namespace seq66
{

sooperlooper_engine_monitor::sooperlooper_engine_monitor (const config & cfg)
    : m_config(cfg)
{
}

void
sooperlooper_engine_monitor::set_config (const config & cfg)
{
    m_config = cfg;
}

const sooperlooper_engine_monitor::config &
sooperlooper_engine_monitor::get_config () const
{
    return m_config;
}

void
sooperlooper_engine_monitor::ping_sent ()
{
    m_last_ping_sent = std::chrono::steady_clock::now();
    if (m_state == engine_state::disabled)
        m_state = engine_state::starting;
}

void
sooperlooper_engine_monitor::ping_reply (const std::string & version,
                                          int loop_count)
{
    m_version = version;
    m_loop_count = loop_count;
    m_last_ping_reply = std::chrono::steady_clock::now();
    m_missed_pings = 0;
    ++m_successful_pings;

    if (m_state == engine_state::starting)
        m_state = engine_state::reconciling;
    else if (m_state == engine_state::stale)
        m_state = engine_state::ready;
    else if (m_state == engine_state::restarting)
        m_state = engine_state::reconciling;
}

void
sooperlooper_engine_monitor::ping_missed ()
{
    ++m_missed_pings;
}

engine_state
sooperlooper_engine_monitor::evaluate ()
{
    auto now = std::chrono::steady_clock::now();

    switch (m_state)
    {
        case engine_state::disabled:
            break;

        case engine_state::starting:
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_last_ping_sent).count();
            if (elapsed > m_config.startup_deadline_ms)
                m_state = engine_state::engine_offline;
            break;
        }

        case engine_state::reconciling:
            // Transition to ready is done externally after verification.
            break;

        case engine_state::ready:
        {
            if (m_missed_pings >= m_config.stale_threshold)
                m_state = engine_state::stale;
            break;
        }

        case engine_state::stale:
        {
            if (m_missed_pings >= m_config.stale_threshold * 2)
                m_state = engine_state::engine_offline;
            break;
        }

        case engine_state::engine_offline:
            break;

        case engine_state::restarting:
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_last_ping_sent).count();
            if (elapsed > m_config.startup_deadline_ms)
                m_state = engine_state::engine_offline;
            break;
        }
    }

    return m_state;
}

engine_state
sooperlooper_engine_monitor::state () const
{
    return m_state;
}

void
sooperlooper_engine_monitor::force_state (engine_state s)
{
    m_state = s;
}

const std::string &
sooperlooper_engine_monitor::version () const
{
    return m_version;
}

int
sooperlooper_engine_monitor::loop_count () const
{
    return m_loop_count;
}

bool
sooperlooper_engine_monitor::is_ready () const
{
    return m_state == engine_state::ready;
}

bool
sooperlooper_engine_monitor::is_unreachable () const
{
    return m_state == engine_state::stale ||
           m_state == engine_state::engine_offline;
}

void
sooperlooper_engine_monitor::reset ()
{
    m_state = engine_state::disabled;
    m_version.clear();
    m_loop_count = 0;
    m_missed_pings = 0;
    m_successful_pings = 0;
}

} // namespace seq66

/*
 * sooperlooper_engine_monitor.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
