/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_readiness_gate_test.cpp
 *
 *  Tests for the readiness gate evaluator.
 */

#include "audio/sooperlooper_readiness_gate.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>

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

/* Helpers */
static seq66::backend_probe_result
usable_backend ()
{
    seq66::backend_probe_result r;
    r.capability = seq66::backend_capability::usable_native_jack;
    r.implementation = "native";
    r.server_reachable = true;
    return r;
}

static seq66::backend_probe_result
unavailable_backend ()
{
    seq66::backend_probe_result r;
    r.capability = seq66::backend_capability::backend_unavailable;
    r.error = "no JACK";
    return r;
}

static seq66::backend_probe_result
error_backend ()
{
    seq66::backend_probe_result r;
    r.capability = seq66::backend_capability::probe_error;
    r.error = "dlopen failed";
    return r;
}

static seq66::jack_topology_result
complete_topology ()
{
    seq66::jack_topology_result t;
    t.complete = true;
    t.summary = "complete";
    return t;
}

static seq66::jack_topology_result
incomplete_topology ()
{
    seq66::jack_topology_result t;
    t.complete = false;
    t.missing_connections.push_back("seq66:cap1 -> sl:in1");
    t.summary = "incomplete";
    return t;
}

/* Tests */

static void
test_all_ready ()
{
    std::cout << "\n--- All conditions ready ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;
    auto r = gate.evaluate(usable_backend(), true,
                           seq66::engine_state::ready,
                           complete_topology(), 1);
    CHECK(r.readiness == seq66::engine_readiness::ready, "ready");
    CHECK(r.conditions.size() == 4, "4 conditions checked");
    for (const auto & c : r.conditions)
        CHECK(c.satisfied, (c.name + " satisfied").c_str());
}

static void
test_backend_unavailable ()
{
    std::cout << "\n--- Backend unavailable ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;
    auto r = gate.evaluate(unavailable_backend(), true,
                           seq66::engine_state::ready,
                           complete_topology(), 1);
    CHECK(r.readiness == seq66::engine_readiness::backend_unavailable,
          "backend_unavailable");
    CHECK(! r.conditions[0].satisfied, "backend not satisfied");
    CHECK(gate.is_backend_unavailable(unavailable_backend()),
          "is_backend_unavailable");
}

static void
test_backend_error ()
{
    std::cout << "\n--- Backend error ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;
    auto r = gate.evaluate(error_backend(), true,
                           seq66::engine_state::ready,
                           complete_topology(), 1);
    CHECK(r.readiness == seq66::engine_readiness::backend_unavailable,
          "backend_unavailable on error");
}

static void
test_child_not_alive ()
{
    std::cout << "\n--- Child not alive ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;
    auto r = gate.evaluate(usable_backend(), false,
                           seq66::engine_state::ready,
                           complete_topology(), 1);
    CHECK(r.readiness == seq66::engine_readiness::degraded, "degraded");
    CHECK(! r.conditions[1].satisfied, "process not satisfied");
}

static void
test_osc_not_ready ()
{
    std::cout << "\n--- OSC not ready (starting) ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;
    auto r = gate.evaluate(usable_backend(), true,
                           seq66::engine_state::starting,
                           complete_topology(), 1);
    CHECK(r.readiness == seq66::engine_readiness::degraded, "degraded");
    CHECK(! r.conditions[2].satisfied, "osc not satisfied");
}

static void
test_osc_stale ()
{
    std::cout << "\n--- OSC stale ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;
    auto r = gate.evaluate(usable_backend(), true,
                           seq66::engine_state::stale,
                           complete_topology(), 1);
    CHECK(r.readiness == seq66::engine_readiness::degraded, "degraded");
}

static void
test_topology_incomplete ()
{
    std::cout << "\n--- Topology incomplete ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;
    auto r = gate.evaluate(usable_backend(), true,
                           seq66::engine_state::ready,
                           incomplete_topology(), 1);
    CHECK(r.readiness == seq66::engine_readiness::degraded, "degraded");
    CHECK(! r.conditions[3].satisfied, "topology not satisfied");
}

static void
test_nothing_ready ()
{
    std::cout << "\n--- Nothing ready ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;
    auto r = gate.evaluate(unavailable_backend(), false,
                           seq66::engine_state::disabled,
                           incomplete_topology(), 0);
    CHECK(r.readiness == seq66::engine_readiness::backend_unavailable,
          "backend_unavailable takes precedence");
}

static void
test_is_ready ()
{
    std::cout << "\n--- is_ready convenience ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;
    CHECK(gate.is_ready(usable_backend(), true,
                        seq66::engine_state::ready,
                        complete_topology(), 1),
          "is_ready when all conditions met");
    CHECK(! gate.is_ready(usable_backend(), false,
                          seq66::engine_state::ready,
                          complete_topology(), 1),
          "not is_ready when child dead");
}

static void
test_generation_recorded ()
{
    std::cout << "\n--- Generation recorded ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;
    auto r = gate.evaluate(usable_backend(), true,
                           seq66::engine_state::ready,
                           complete_topology(), 42);
    CHECK(r.generation == 42, "generation is 42");
}

static void
test_readiness_enum ()
{
    std::cout << "\n--- Readiness enum ---" << std::endl;
    CHECK(static_cast<int>(seq66::engine_readiness::not_ready) == 0,
          "not_ready is 0");
    CHECK(static_cast<int>(seq66::engine_readiness::ready) == 1,
          "ready is 1");
    CHECK(static_cast<int>(seq66::engine_readiness::degraded) == 2,
          "degraded is 2");
    CHECK(static_cast<int>(seq66::engine_readiness::backend_unavailable) == 3,
          "backend_unavailable is 3");
}

static void
test_condition_names ()
{
    std::cout << "\n--- Condition names ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;
    auto r = gate.evaluate(usable_backend(), true,
                           seq66::engine_state::ready,
                           complete_topology(), 1);
    CHECK(r.conditions[0].name == "backend", "condition 0 is backend");
    CHECK(r.conditions[1].name == "process", "condition 1 is process");
    CHECK(r.conditions[2].name == "osc", "condition 2 is osc");
    CHECK(r.conditions[3].name == "topology", "condition 3 is topology");
}

static void
test_summary ()
{
    std::cout << "\n--- Summary ---" << std::endl;
    seq66::sooperlooper_readiness_gate gate;

    auto r1 = gate.evaluate(usable_backend(), true,
                            seq66::engine_state::ready,
                            complete_topology(), 1);
    CHECK(r1.summary.find("ready") != std::string::npos,
          "ready summary");

    auto r2 = gate.evaluate(usable_backend(), false,
                            seq66::engine_state::ready,
                            complete_topology(), 1);
    CHECK(r2.summary.find("degraded") != std::string::npos,
          "degraded summary");
}

int
main ()
{
    std::cout << "=== Readiness gate tests ===" << std::endl;

    test_all_ready();
    test_backend_unavailable();
    test_backend_error();
    test_child_not_alive();
    test_osc_not_ready();
    test_osc_stale();
    test_topology_incomplete();
    test_nothing_ready();
    test_is_ready();
    test_generation_recorded();
    test_readiness_enum();
    test_condition_names();
    test_summary();

    std::cout << std::endl;
    if (s_failures == 0)
    {
        std::cout << "All readiness gate tests passed." << std::endl;
        return 0;
    }
    else
    {
        std::cerr << s_failures << " test(s) FAILED." << std::endl;
        return 1;
    }
}
