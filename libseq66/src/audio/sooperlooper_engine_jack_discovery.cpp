/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_engine_jack_discovery.cpp
 *
 *  JACK discovery and routing implementation.
 */

#include "audio/sooperlooper_engine_jack_discovery.hpp"

#include <algorithm>

namespace seq66
{

/* ------------------------------------------------------------------ */
/*  Construction                                                       */
/* ------------------------------------------------------------------ */

sooperlooper_jack_discovery::sooperlooper_jack_discovery ()
    : m_adapter(nullptr)
{
}

sooperlooper_jack_discovery::sooperlooper_jack_discovery (
    std::shared_ptr<jack_graph_adapter> adapter)
    : m_adapter(std::move(adapter))
{
}

sooperlooper_jack_discovery::~sooperlooper_jack_discovery ()
{
}

/* ------------------------------------------------------------------ */
/*  Expected connections                                                */
/* ------------------------------------------------------------------ */

void
sooperlooper_jack_discovery::add_expected_connection (
    const jack_expected_connection & conn)
{
    m_expected.push_back(conn);
}

const std::vector<jack_expected_connection> &
sooperlooper_jack_discovery::expected_connections () const
{
    return m_expected;
}

/* ------------------------------------------------------------------ */
/*  Discovery                                                          */
/* ------------------------------------------------------------------ */

int
sooperlooper_jack_discovery::discover ()
{
    if (! m_adapter)
    {
        m_last_error = "no JACK graph adapter configured";
        return 0;
    }

    m_discovered.clear();
    m_last_error.clear();
    ++m_graph_generation;

    auto client_names = m_adapter->get_clients();

    for (const auto & cname : client_names)
    {
        jack_client_info ci;
        ci.name = cname;
        ci.active = true;

        auto port_names = m_adapter->get_ports(cname);
        for (const auto & pname : port_names)
        {
            jack_port_info pi;
            pi.client_name = cname;
            pi.port_name = pname;
            pi.status = jack_port_status::discovered;

            /* Heuristic: input ports often end with " in" or start with
             * "in"; output ports with " out" or "out".  This is a
             * simplification — real JACK API uses port flags. */
            if (pname.find("in") != std::string::npos)
                pi.is_input = true;
            if (pname.find("out") != std::string::npos)
                pi.is_output = true;

            ci.ports.push_back(pi);
        }

        m_discovered.push_back(ci);
    }

    return static_cast<int>(m_discovered.size());
}

/* ------------------------------------------------------------------ */
/*  Connection management                                               */
/* ------------------------------------------------------------------ */

int
sooperlooper_jack_discovery::connect_all ()
{
    if (! m_adapter)
    {
        m_last_error = "no JACK graph adapter configured";
        return 0;
    }

    int new_connections = 0;

    for (const auto & expected : m_expected)
    {
        if (connection_exists(expected.source_client, expected.source_port,
                              expected.target_client, expected.target_port))
        {
            continue;  /* Already connected — idempotent */
        }

        /* Verify both ports exist */
        if (! m_adapter->port_exists(expected.source_client,
                                     expected.source_port))
        {
            m_last_error = "source port not found: " +
                           expected.source_client + ":" +
                           expected.source_port;
            continue;
        }

        if (! m_adapter->port_exists(expected.target_client,
                                     expected.target_port))
        {
            m_last_error = "target port not found: " +
                           expected.target_client + ":" +
                           expected.target_port;
            continue;
        }

        /* Attempt connection */
        if (m_adapter->connect(expected.source_client, expected.source_port,
                               expected.target_client, expected.target_port))
        {
            jack_connection conn;
            conn.source_client = expected.source_client;
            conn.source_port = expected.source_port;
            conn.target_client = expected.target_client;
            conn.target_port = expected.target_port;
            conn.status = jack_connection_status::established;
            m_connections.push_back(conn);
            ++new_connections;
        }
        else
        {
            jack_connection conn;
            conn.source_client = expected.source_client;
            conn.source_port = expected.source_port;
            conn.target_client = expected.target_client;
            conn.target_port = expected.target_port;
            conn.status = jack_connection_status::failed;
            m_connections.push_back(conn);

            m_last_error = "connection failed: " +
                           expected.source_client + ":" +
                           expected.source_port + " -> " +
                           expected.target_client + ":" +
                           expected.target_port;
        }
    }

    return new_connections;
}

int
sooperlooper_jack_discovery::disconnect_all ()
{
    if (! m_adapter)
        return 0;

    int disconnected = 0;

    for (auto & conn : m_connections)
    {
        if (conn.status == jack_connection_status::established)
        {
            m_adapter->disconnect(conn.source_client, conn.source_port,
                                  conn.target_client, conn.target_port);
            conn.status = jack_connection_status::disconnected;
            ++disconnected;
        }
    }

    return disconnected;
}

/* ------------------------------------------------------------------ */
/*  Topology checking                                                   */
/* ------------------------------------------------------------------ */

jack_topology_result
sooperlooper_jack_discovery::check_topology () const
{
    jack_topology_result result;
    result.complete = true;

    for (const auto & expected : m_expected)
    {
        bool found = false;
        for (const auto & conn : m_connections)
        {
            if (conn.source_client == expected.source_client &&
                conn.source_port == expected.source_port &&
                conn.target_client == expected.target_client &&
                conn.target_port == expected.target_port &&
                conn.status == jack_connection_status::established)
            {
                found = true;
                break;
            }
        }

        if (! found)
        {
            result.complete = false;
            result.missing_connections.push_back(
                expected.source_client + ":" + expected.source_port +
                " -> " +
                expected.target_client + ":" + expected.target_port);
        }
    }

    if (result.complete)
        result.summary = "topology complete";
    else
        result.summary = "incomplete: " +
            std::to_string(result.missing_connections.size()) +
            " missing connection(s)";

    return result;
}

/* ------------------------------------------------------------------ */
/*  Graph change detection                                              */
/* ------------------------------------------------------------------ */

bool
sooperlooper_jack_discovery::graph_changed ()
{
    if (! m_adapter)
        return false;

    /* Simple heuristic: compare current client list with discovered.
     * A real implementation would use JACK graph callback. */
    auto current = m_adapter->get_clients();
    std::vector<std::string> known;
    for (const auto & ci : m_discovered)
        known.push_back(ci.name);

    std::sort(current.begin(), current.end());
    std::sort(known.begin(), known.end());

    return current != known;
}

/* ------------------------------------------------------------------ */
/*  Reconnection                                                       */
/* ------------------------------------------------------------------ */

int
sooperlooper_jack_discovery::reconnect ()
{
    /* Clear existing connection records to detect externally lost
     * connections.  connect_all() will re-establish them. */
    m_connections.clear();
    discover();
    return connect_all();
}

/* ------------------------------------------------------------------ */
/*  Queries                                                             */
/* ------------------------------------------------------------------ */

const std::vector<jack_client_info> &
sooperlooper_jack_discovery::discovered_clients () const
{
    return m_discovered;
}

const std::vector<jack_connection> &
sooperlooper_jack_discovery::connections () const
{
    return m_connections;
}

const std::string &
sooperlooper_jack_discovery::last_error () const
{
    return m_last_error;
}

int
sooperlooper_jack_discovery::connected_count () const
{
    int count = 0;
    for (const auto & c : m_connections)
    {
        if (c.status == jack_connection_status::established)
            ++count;
    }
    return count;
}

int
sooperlooper_jack_discovery::missing_count () const
{
    return check_topology().missing_connections.size();
}

/* ------------------------------------------------------------------ */
/*  Private helpers                                                     */
/* ------------------------------------------------------------------ */

const jack_client_info *
sooperlooper_jack_discovery::find_client (const std::string & name) const
{
    for (const auto & ci : m_discovered)
    {
        if (ci.name == name)
            return &ci;
    }
    return nullptr;
}

bool
sooperlooper_jack_discovery::connection_exists (
    const std::string & src_client,
    const std::string & src_port,
    const std::string & tgt_client,
    const std::string & tgt_port) const
{
    for (const auto & c : m_connections)
    {
        if (c.source_client == src_client &&
            c.source_port == src_port &&
            c.target_client == tgt_client &&
            c.target_port == tgt_port &&
            c.status == jack_connection_status::established)
        {
            return true;
        }
    }
    return false;
}

} // namespace seq66

/*
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
