#if ! defined SEQ66_SOOPERLOOPER_JACK_DISCOVERY_HPP
#define SEQ66_SOOPERLOOPER_JACK_DISCOVERY_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_engine_jack_discovery.hpp
 *
 *  JACK client/port discovery and Seq66-owned routing for SooperLooper
 *  integration.
 *
 *  Discovers expected JACK clients (e.g. SooperLooper) and their ports,
 *  establishes idempotent connections owned by Seq66, reports incomplete
 *  topology, and detects graph changes for reconnection.
 *
 *  The JACK graph adapter is injectable: callers provide a
 *  jack_graph_interface so that tests can supply fake implementations
 *  without requiring a real JACK server.
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace seq66
{

/**
 *  Status of a JACK port.
 */
enum class jack_port_status : int
{
    absent,         /**< Port does not exist in the graph.            */
    discovered,     /**< Port exists but is not yet connected.        */
    connected,      /**< Port exists and connection is established.   */
    disconnected    /**< Port existed but connection was lost.        */
};

/**
 *  Status of a JACK connection.
 */
enum class jack_connection_status : int
{
    pending,        /**< Connection not yet attempted.                */
    established,    /**< Connection is active.                        */
    failed,         /**< Connection attempt failed.                   */
    disconnected    /**< Connection was lost.                         */
};

/**
 *  A discovered JACK port.
 */
struct jack_port_info
{
    std::string client_name;        /**< JACK client name.              */
    std::string port_name;          /**< Port name within client.       */
    int port_index{-1};             /**< JACK port index.               */
    bool is_input{false};           /**< True if this is an input port. */
    bool is_output{false};          /**< True if this is an output port.*/
    jack_port_status status{jack_port_status::absent};
};

/**
 *  A discovered JACK client.
 */
struct jack_client_info
{
    std::string name;               /**< Client name.                   */
    std::vector<jack_port_info> ports;  /**< Discovered ports.         */
    bool active{false};             /**< True if client is alive.       */
};

/**
 *  A Seq66-owned JACK connection.
 */
struct jack_connection
{
    std::string source_client;      /**< Source client name.            */
    std::string source_port;        /**< Source port name.              */
    std::string target_client;      /**< Target client name.            */
    std::string target_port;        /**< Target port name.              */
    jack_connection_status status{jack_connection_status::pending};
};

/**
 *  Topology completeness result.
 */
struct jack_topology_result
{
    bool complete{true};            /**< True if all expected connections exist. */
    std::vector<std::string> missing_connections;  /**< Missing connections.     */
    std::vector<std::string> unexpected_clients;   /**< Unexpected clients.      */
    std::string summary;            /**< Human-readable summary.        */
};

/**
 *  Expected connection specification.
 *
 *  Defines what connections Seq66 should maintain.
 */
struct jack_expected_connection
{
    std::string source_client;      /**< Source client name pattern.    */
    std::string source_port;        /**< Source port name pattern.      */
    std::string target_client;      /**< Target client name pattern.    */
    std::string target_port;        /**< Target port name pattern.      */
};

/**
 *  Injectable JACK graph interface.
 *
 *  Abstraction over JACK graph operations.  The production implementation
 *  uses the JACK client API.  Tests inject a fake that records calls.
 */
class jack_graph_adapter
{
public:
    virtual ~jack_graph_adapter () = default;

    /**
     *  Get the list of active JACK clients.
     */
    virtual std::vector<std::string> get_clients () = 0;

    /**
     *  Get the ports for a specific client.
     *
     *  \param client_name  Client to query.
     *  \return List of port names.
     */
    virtual std::vector<std::string> get_ports (
        const std::string & client_name) = 0;

    /**
     *  Check if a port exists.
     *
     *  \param client_name  Client name.
     *  \param port_name    Port name.
     *  \return true if the port exists.
     */
    virtual bool port_exists (
        const std::string & client_name,
        const std::string & port_name) = 0;

    /**
     *  Connect two ports.
     *
     *  \return true on success.
     */
    virtual bool connect (
        const std::string & source_client,
        const std::string & source_port,
        const std::string & target_client,
        const std::string & target_port) = 0;

    /**
     *  Disconnect two ports.
     *
     *  \return true on success.
     */
    virtual bool disconnect (
        const std::string & source_client,
        const std::string & source_port,
        const std::string & target_client,
        const std::string & target_port) = 0;

    /**
     *  Check if two ports are connected.
     */
    virtual bool is_connected (
        const std::string & source_client,
        const std::string & source_port,
        const std::string & target_client,
        const std::string & target_port) = 0;
};

/**
 *  JACK discovery and routing manager.
 *
 *  Discovers expected JACK clients/ports, establishes idempotent
 *  connections, reports incomplete topology, and detects graph changes.
 *
 *  Thread safety: all methods are safe to call from a single
 *  coordinator thread.
 */
class sooperlooper_jack_discovery
{
public:
    sooperlooper_jack_discovery ();
    explicit sooperlooper_jack_discovery (
        std::shared_ptr<jack_graph_adapter> adapter);
    ~sooperlooper_jack_discovery ();

    sooperlooper_jack_discovery
        (const sooperlooper_jack_discovery &) = delete;
    sooperlooper_jack_discovery & operator =
        (const sooperlooper_jack_discovery &) = delete;

    /**
     *  Add an expected connection specification.
     */
    void add_expected_connection (const jack_expected_connection & conn);

    /**
     *  Get all expected connections.
     */
    const std::vector<jack_expected_connection> &
    expected_connections () const;

    /**
     *  Discover all expected clients and their ports.
     *
     *  \return Number of clients discovered.
     */
    int discover ();

    /**
     *  Establish all expected connections that are not yet established.
     *
     *  Idempotent: repeated calls produce the same result.
     *
     *  \return Number of new connections established.
     */
    int connect_all ();

    /**
     *  Disconnect all Seq66-owned connections.
     *
     *  \return Number of connections disconnected.
     */
    int disconnect_all ();

    /**
     *  Check topology completeness against expected connections.
     *
     *  \return Topology result with missing/unexpected items.
     */
    jack_topology_result check_topology () const;

    /**
     *  Detect if the JACK graph has changed since the last discover().
     *
     *  \return true if the graph changed.
     */
    bool graph_changed ();

    /**
     *  Reconnect after graph changes.
     *
     *  Discovers the graph, checks topology, and reconnects missing
     *  connections.
     *
     *  \return Number of connections re-established.
     */
    int reconnect ();

    /**
     *  Get all discovered clients.
     */
    const std::vector<jack_client_info> & discovered_clients () const;

    /**
     *  Get all established connections.
     */
    const std::vector<jack_connection> & connections () const;

    /**
     *  Get the last error message.
     */
    const std::string & last_error () const;

    /**
     *  Get the number of connections currently established.
     */
    int connected_count () const;

    /**
     *  Get the number of missing connections.
     */
    int missing_count () const;

private:
    /** Find a client by name in discovered_clients. */
    const jack_client_info * find_client (
        const std::string & name) const;

    /** Check if a connection already exists. */
    bool connection_exists (
        const std::string & src_client,
        const std::string & src_port,
        const std::string & tgt_client,
        const std::string & tgt_port) const;

    std::shared_ptr<jack_graph_adapter> m_adapter;
    std::vector<jack_expected_connection> m_expected;
    std::vector<jack_client_info> m_discovered;
    std::vector<jack_connection> m_connections;
    std::string m_last_error;
    std::uint64_t m_graph_generation{0};
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_JACK_DISCOVERY_HPP

/*
 * sooperlooper_engine_jack_discovery.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
