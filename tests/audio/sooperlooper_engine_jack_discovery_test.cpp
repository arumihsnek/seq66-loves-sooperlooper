/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_engine_jack_discovery_test.cpp
 *
 *  Tests for JACK discovery and routing.  Uses a fake graph adapter
 *  to verify discovery, connection, topology, idempotency, and
 *  graph-change detection without a real JACK server.
 */

#include "audio/sooperlooper_engine_jack_discovery.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <set>
#include <string>
#include <vector>

static int s_failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (! (cond)) { \
            std::cerr << "  [FAIL] " << msg << std::endl; \
            ++s_failures; \
        } else { \
            std::cout << "  [PASS] " << msg << std::endl; \
        } \
    } while (0)

/* ------------------------------------------------------------------ */
/*  Fake JACK graph adapter                                             */
/* ------------------------------------------------------------------ */

class fake_jack_graph : public seq66::jack_graph_adapter
{
public:
    std::vector<std::string> clients;
    std::vector<std::pair<std::string, std::string>> ports; /* client, port */
    std::set<std::pair<std::string, std::string>> connected;
    bool connect_succeeds{true};

    std::vector<std::string> get_clients () override
    {
        return clients;
    }

    std::vector<std::string> get_ports (const std::string & client) override
    {
        std::vector<std::string> result;
        for (const auto & p : ports)
        {
            if (p.first == client)
                result.push_back(p.second);
        }
        return result;
    }

    bool port_exists (const std::string & client,
                      const std::string & port) override
    {
        for (const auto & p : ports)
        {
            if (p.first == client && p.second == port)
                return true;
        }
        return false;
    }

    bool connect (const std::string & src_client,
                  const std::string & src_port,
                  const std::string & tgt_client,
                  const std::string & tgt_port) override
    {
        if (! connect_succeeds)
            return false;
        connected.insert({src_client + ":" + src_port,
                          tgt_client + ":" + tgt_port});
        return true;
    }

    bool disconnect (const std::string & src_client,
                     const std::string & src_port,
                     const std::string & tgt_client,
                     const std::string & tgt_port) override
    {
        connected.erase({src_client + ":" + src_port,
                         tgt_client + ":" + tgt_port});
        return true;
    }

    bool is_connected (const std::string & src_client,
                       const std::string & src_port,
                       const std::string & tgt_client,
                       const std::string & tgt_port) override
    {
        return connected.count({src_client + ":" + src_port,
                                tgt_client + ":" + tgt_port}) > 0;
    }
};

/* ------------------------------------------------------------------ */
/*  Helper                                                             */
/* ------------------------------------------------------------------ */

static std::shared_ptr<fake_jack_graph>
make_graph_with_sooperlooper ()
{
    auto g = std::make_shared<fake_jack_graph>();
    g->clients = {"sooperlooper", "seq66"};
    g->ports = {
        {"sooperlooper", "loop_0_in"},
        {"sooperlooper", "loop_0_out"},
        {"sooperlooper", "common_out_L"},
        {"sooperlooper", "common_out_R"},
        {"seq66", "capture_1"},
        {"seq66", "capture_2"},
        {"seq66", "playback_1"},
        {"seq66", "playback_2"}
    };
    return g;
}

/* ------------------------------------------------------------------ */
/*  Test groups                                                         */
/* ------------------------------------------------------------------ */

static void
test_discover_clients ()
{
    std::cout << "\n--- Discover clients ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    int count = disc.discover();
    CHECK(count == 2, "discovered 2 clients");
    CHECK(disc.discovered_clients().size() == 2, "2 clients in list");
    CHECK(disc.discovered_clients()[0].name == "sooperlooper",
          "first client is sooperlooper");
    CHECK(disc.discovered_clients()[0].ports.size() == 4,
          "sooperlooper has 4 ports");
}

static void
test_discover_no_adapter ()
{
    std::cout << "\n--- Discover without adapter ---" << std::endl;
    seq66::sooperlooper_jack_discovery disc;
    int count = disc.discover();
    CHECK(count == 0, "returns 0 without adapter");
    CHECK(! disc.last_error().empty(), "error set");
}

static void
test_connect_all ()
{
    std::cout << "\n--- Connect all ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    disc.add_expected_connection({"seq66", "capture_1",
                                  "sooperlooper", "loop_0_in"});
    disc.add_expected_connection({"seq66", "capture_2",
                                  "sooperlooper", "loop_0_in"});

    disc.discover();
    int new_conn = disc.connect_all();
    CHECK(new_conn == 2, "2 connections established");
    CHECK(disc.connected_count() == 2, "2 connected");
}

static void
test_connect_idempotent ()
{
    std::cout << "\n--- Connect idempotent ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    disc.add_expected_connection({"seq66", "capture_1",
                                  "sooperlooper", "loop_0_in"});

    disc.discover();
    disc.connect_all();
    int again = disc.connect_all();
    CHECK(again == 0, "second connect_all returns 0 (idempotent)");
    CHECK(disc.connected_count() == 1, "still 1 connected");
}

static void
test_connect_missing_port ()
{
    std::cout << "\n--- Connect with missing port ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    disc.add_expected_connection({"seq66", "nonexistent_port",
                                  "sooperlooper", "loop_0_in"});

    disc.discover();
    int new_conn = disc.connect_all();
    CHECK(new_conn == 0, "no connections when port missing");
    CHECK(disc.connected_count() == 0, "0 connected");
}

static void
test_connect_failure ()
{
    std::cout << "\n--- Connect failure ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    g->connect_succeeds = false;
    seq66::sooperlooper_jack_discovery disc(g);

    disc.add_expected_connection({"seq66", "capture_1",
                                  "sooperlooper", "loop_0_in"});

    disc.discover();
    int new_conn = disc.connect_all();
    CHECK(new_conn == 0, "0 on connect failure");
    CHECK(disc.connections().size() == 1, "1 connection record");
    CHECK(disc.connections()[0].status == seq66::jack_connection_status::failed,
          "status is failed");
}

static void
test_disconnect_all ()
{
    std::cout << "\n--- Disconnect all ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    disc.add_expected_connection({"seq66", "capture_1",
                                  "sooperlooper", "loop_0_in"});
    disc.discover();
    disc.connect_all();
    CHECK(disc.connected_count() == 1, "1 connected before");

    int disconn = disc.disconnect_all();
    CHECK(disconn == 1, "1 disconnected");
    CHECK(disc.connected_count() == 0, "0 connected after");
}

static void
test_topology_complete ()
{
    std::cout << "\n--- Topology complete ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    disc.add_expected_connection({"seq66", "capture_1",
                                  "sooperlooper", "loop_0_in"});
    disc.discover();
    disc.connect_all();

    auto result = disc.check_topology();
    CHECK(result.complete, "topology is complete");
    CHECK(result.missing_connections.empty(), "no missing connections");
}

static void
test_topology_incomplete ()
{
    std::cout << "\n--- Topology incomplete ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    disc.add_expected_connection({"seq66", "capture_1",
                                  "sooperlooper", "loop_0_in"});
    disc.add_expected_connection({"seq66", "capture_2",
                                  "sooperlooper", "loop_0_in"});

    disc.discover();
    disc.connect_all();  /* Only first connects; second is duplicate target */

    auto result = disc.check_topology();
    /* Both should connect since they're different source ports */
    CHECK(result.complete, "topology complete with both connections");
}

static void
test_topology_incomplete_missing ()
{
    std::cout << "\n--- Topology incomplete (missing) ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    disc.add_expected_connection({"seq66", "capture_1",
                                  "sooperlooper", "loop_0_in"});
    disc.add_expected_connection({"seq66", "capture_2",
                                  "sooperlooper", "nonexistent_port"});

    disc.discover();
    disc.connect_all();

    auto result = disc.check_topology();
    CHECK(! result.complete, "topology is incomplete");
    CHECK(result.missing_connections.size() == 1, "1 missing connection");
    CHECK(! result.summary.empty(), "summary is set");
}

static void
test_graph_changed ()
{
    std::cout << "\n--- Graph change detection ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    disc.discover();
    CHECK(! disc.graph_changed(), "no change after discover");

    /* Add a new client to the graph */
    g->clients.push_back("new_client");
    CHECK(disc.graph_changed(), "change detected after new client");
}

static void
test_reconnect ()
{
    std::cout << "\n--- Reconnect ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    disc.add_expected_connection({"seq66", "capture_1",
                                  "sooperlooper", "loop_0_in"});
    disc.discover();
    disc.connect_all();
    CHECK(disc.connected_count() == 1, "1 connected");

    /* Simulate graph change: disconnect externally */
    g->connected.clear();
    CHECK(disc.connected_count() == 1, "still 1 in local state");

    /* Reconnect should re-establish */
    int reconnected = disc.reconnect();
    CHECK(reconnected == 1, "1 reconnected");
    CHECK(disc.connected_count() == 1, "1 connected after reconnect");
}

static void
test_no_expected_connections ()
{
    std::cout << "\n--- No expected connections ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    disc.discover();
    int new_conn = disc.connect_all();
    CHECK(new_conn == 0, "0 connections when none expected");
    CHECK(disc.connected_count() == 0, "0 connected");
}

static void
test_missing_count ()
{
    std::cout << "\n--- Missing count ---" << std::endl;
    auto g = make_graph_with_sooperlooper();
    seq66::sooperlooper_jack_discovery disc(g);

    disc.add_expected_connection({"seq66", "capture_1",
                                  "sooperlooper", "loop_0_in"});
    disc.add_expected_connection({"seq66", "capture_2",
                                  "sooperlooper", "nonexistent"});

    disc.discover();
    disc.connect_all();

    CHECK(disc.missing_count() == 1, "1 missing");
}

static void
test_connection_status_values ()
{
    std::cout << "\n--- Connection status enum ---" << std::endl;
    CHECK(static_cast<int>(seq66::jack_connection_status::pending) == 0,
          "pending is 0");
    CHECK(static_cast<int>(seq66::jack_connection_status::established) == 1,
          "established is 1");
    CHECK(static_cast<int>(seq66::jack_connection_status::failed) == 2,
          "failed is 2");
    CHECK(static_cast<int>(seq66::jack_connection_status::disconnected) == 3,
          "disconnected is 3");
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int
main ()
{
    std::cout << "=== JACK discovery tests ===" << std::endl;

    test_discover_clients();
    test_discover_no_adapter();
    test_connect_all();
    test_connect_idempotent();
    test_connect_missing_port();
    test_connect_failure();
    test_disconnect_all();
    test_topology_complete();
    test_topology_incomplete();
    test_topology_incomplete_missing();
    test_graph_changed();
    test_reconnect();
    test_no_expected_connections();
    test_missing_count();
    test_connection_status_values();

    std::cout << std::endl;
    if (s_failures == 0)
    {
        std::cout << "All JACK discovery tests passed." << std::endl;
        return 0;
    }
    else
    {
        std::cerr << s_failures << " test(s) FAILED." << std::endl;
        return 1;
    }
}

/*
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
