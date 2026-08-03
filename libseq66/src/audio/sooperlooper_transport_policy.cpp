/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_transport_policy.cpp
 *
 *  Transport policy implementation.
 */

#include "audio/sooperlooper_transport_policy.hpp"

#include <algorithm>
#include <cstring>

namespace seq66
{

const transport_config &
sooperlooper_transport_policy::config () const
{
    return m_config;
}

void
sooperlooper_transport_policy::set_config (const transport_config & cfg)
{
    m_config = cfg;
}

int
sooperlooper_transport_policy::sync_source_value () const
{
    switch (m_config.sync_source)
    {
        case transport_sync_source::none:      return 0;
        case transport_sync_source::jack:      return -1;
        case transport_sync_source::midi:      return -2;
        case transport_sync_source::internal:  return -3;
    }
    return 0;
}

bool
sooperlooper_transport_policy::set_sync_source (const std::string & name)
{
    transport_sync_source src;
    if (try_parse(name, src))
    {
        m_config.sync_source = src;
        return true;
    }
    return false;
}

void
sooperlooper_transport_policy::set_sync_source_from_value (int value)
{
    if (value <= -3)
        m_config.sync_source = transport_sync_source::internal;
    else if (value == -2)
        m_config.sync_source = transport_sync_source::midi;
    else if (value == -1)
        m_config.sync_source = transport_sync_source::jack;
    else
        m_config.sync_source = transport_sync_source::none;
}

transport_state
sooperlooper_transport_policy::state () const
{
    return m_state;
}

bool
sooperlooper_transport_policy::request_start ()
{
    if (m_state == transport_state::stopped ||
        m_state == transport_state::stopping)
    {
        m_state = transport_state::playing;
        return true;
    }
    return false;
}

bool
sooperlooper_transport_policy::request_stop ()
{
    if (m_state == transport_state::playing ||
        m_state == transport_state::starting)
    {
        m_state = transport_state::stopped;
        return true;
    }
    return false;
}

bool
sooperlooper_transport_policy::is_jack_sync () const
{
    return m_config.sync_source == transport_sync_source::jack;
}

bool
sooperlooper_transport_policy::is_midi_sync () const
{
    return m_config.sync_source == transport_sync_source::midi;
}

bool
sooperlooper_transport_policy::is_no_sync () const
{
    return m_config.sync_source == transport_sync_source::none;
}

const char *
to_string (transport_sync_source src)
{
    switch (src)
    {
        case transport_sync_source::none:     return "none";
        case transport_sync_source::jack:     return "jack";
        case transport_sync_source::midi:     return "midi";
        case transport_sync_source::internal: return "internal";
    }
    return "unknown";
}

const char *
to_string (transport_state state)
{
    switch (state)
    {
        case transport_state::stopped:  return "stopped";
        case transport_state::starting: return "starting";
        case transport_state::playing:  return "playing";
        case transport_state::stopping: return "stopping";
    }
    return "unknown";
}

bool
try_parse (const std::string & name, transport_sync_source & src)
{
    if (name == "none")     { src = transport_sync_source::none;     return true; }
    if (name == "jack")     { src = transport_sync_source::jack;     return true; }
    if (name == "midi")     { src = transport_sync_source::midi;     return true; }
    if (name == "internal") { src = transport_sync_source::internal; return true; }
    return false;
}

} // namespace seq66

/*
 * sooperlooper_transport_policy.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
