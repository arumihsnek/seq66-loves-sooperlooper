/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_engine_launcher_test.cpp
 *
 *  Tests for the engine launch orchestrator.  Uses fake adapters to verify
 *  the launch sequence, naming policy, generation synchronization, backend
 *  gating, and stale callback rejection without real processes.
 *
 *  Build command (mirrors audio-core.yml):
 *      g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -pthread \
 *          -Ilibseq66/include \
 *          libseq66/src/audio/sooperlooper_process_supervisor.cpp \
 *          libseq66/src/audio/sooperlooper_engine_launcher.cpp \
 *          libseq66/src/audio/sooperlooper_observed_state.cpp \
 *          libseq66/src/audio/sooperlooper_engine_monitor.cpp \
 *          tests/audio/sooperlooper_engine_launcher_test.cpp \
 *          -o .ci/bin/sooperlooper_engine_launcher_test
 */

#include "audio/sooperlooper_engine_launcher.hpp"
#include "audio/sooperlooper_observed_state.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
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
/*  Fake process adapter                                               */
/* ------------------------------------------------------------------ */

class fake_process_adapter : public seq66::process_adapter
{
public:
    bool spawn_succeeds{true};
    bool alive_after_spawn{true};
    int next_pid{42};
    int spawn_count{0};
    int last_spawn_pid{0};

    bool spawn (const std::vector<std::string> &, int & pid) override
    {
        ++spawn_count;
        if (spawn_succeeds)
        {
            pid = next_pid;
            last_spawn_pid = pid;
            return true;
        }
        return false;
    }
    bool send_signal (int, int) override { return true; }
    bool is_alive (int pid) override
    {
        return pid == last_spawn_pid && alive_after_spawn;
    }
    bool wait_exit (int, int) override { return true; }
};

/* ------------------------------------------------------------------ */
/*  Fake launcher adapter                                              */
/* ------------------------------------------------------------------ */

class fake_launcher_adapter : public seq66::engine_launcher_adapter
{
public:
    seq66::backend_probe_result probe_result;
    bool port_available{true};
    std::set<int> reserved_ports;

    seq66::backend_probe_result probe_backend () override
    {
        return probe_result;
    }

    bool is_port_available (int port) override
    {
        return port_available && reserved_ports.find(port) == reserved_ports.end();
    }

    bool reserve_port (int port) override
    {
        if (! is_port_available(port))
            return false;
        reserved_ports.insert(port);
        return true;
    }

    void release_port (int port) override
    {
        reserved_ports.erase(port);
    }
};

/* ------------------------------------------------------------------ */
/*  Helper to create a usable backend probe result                     */
/* ------------------------------------------------------------------ */

static seq66::backend_probe_result
make_usable_probe ()
{
    seq66::backend_probe_result r;
    r.capability = seq66::backend_capability::usable_native_jack;
    r.client_library = "jack";
    r.implementation = "native";
    r.server_info = "JACK 1.9.12";
    r.server_reachable = true;
    return r;
}

static seq66::backend_probe_result
make_unavailable_probe ()
{
    seq66::backend_probe_result r;
    r.capability = seq66::backend_capability::backend_unavailable;
    r.error = "no JACK library found";
    return r;
}

/* ------------------------------------------------------------------ */
/*  Test groups                                                        */
/* ------------------------------------------------------------------ */

static void
test_initial_state ()
{
    std::cout << "\n--- Initial state ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    seq66::sooperlooper_engine_launcher launcher(adapter, proc);

    CHECK(launcher.state() == seq66::launcher_state::idle,
          "initial state is idle");
    CHECK(launcher.generation() == 0, "initial generation is 0");
    CHECK(launcher.last_error().empty(), "no error initially");
}

static void
test_launch_backend_unavailable ()
{
    std::cout << "\n--- Launch with backend unavailable ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    adapter->probe_result = make_unavailable_probe();

    seq66::sooperlooper_engine_launcher launcher(adapter, proc);

    seq66::engine_naming_policy np;
    np.instance_id = "test-instance";
    launcher.set_naming_policy(np);

    auto result = launcher.launch();
    CHECK(result == seq66::launcher_result::backend_unavailable,
          "returns backend_unavailable");
    CHECK(launcher.state() == seq66::launcher_state::idle,
          "state returns to idle");
    CHECK(proc->spawn_count == 0, "no process spawned");
}

static void
test_launch_success ()
{
    std::cout << "\n--- Successful launch ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    adapter->probe_result = make_usable_probe();

    seq66::sooperlooper_engine_launcher launcher(adapter, proc);

    seq66::engine_naming_policy np;
    np.instance_id = "test-instance";
    np.osc_port_base = 9951;
    np.osc_port_range = 100;
    launcher.set_naming_policy(np);

    seq66::supervisor_config scfg;
    scfg.executable_path = "/usr/bin/sooperlooper";
    launcher.set_supervisor_config(scfg);

    auto result = launcher.launch();
    CHECK(result == seq66::launcher_result::success,
          "launch returns success");
    CHECK(launcher.state() == seq66::launcher_state::running,
          "state is running");
    CHECK(launcher.generation() == 1, "generation is 1");
    CHECK(proc->spawn_count == 1, "process spawned once");
    CHECK(launcher.engine_alive(), "engine is alive");
    CHECK(! launcher.current_jack_client_name().empty(),
          "JACK client name is set");
    CHECK(launcher.current_osc_port() >= 9951, "OSC port in range");
    CHECK(launcher.current_osc_port() < 10051, "OSC port in range");
}

static void
test_launch_already_running ()
{
    std::cout << "\n--- Launch when already running ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    adapter->probe_result = make_usable_probe();

    seq66::sooperlooper_engine_launcher launcher(adapter, proc);

    seq66::engine_naming_policy np;
    np.instance_id = "test";
    launcher.set_naming_policy(np);

    seq66::supervisor_config scfg;
    scfg.executable_path = "/usr/bin/sooperlooper";
    launcher.set_supervisor_config(scfg);

    launcher.launch();
    auto result = launcher.launch();
    CHECK(result == seq66::launcher_result::supervisor_failed,
          "second launch fails");
}

static void
test_shutdown ()
{
    std::cout << "\n--- Shutdown ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    adapter->probe_result = make_usable_probe();

    seq66::sooperlooper_engine_launcher launcher(adapter, proc);

    seq66::engine_naming_policy np;
    np.instance_id = "test";
    launcher.set_naming_policy(np);

    seq66::supervisor_config scfg;
    scfg.executable_path = "/usr/bin/sooperlooper";
    launcher.set_supervisor_config(scfg);

    launcher.launch();
    CHECK(launcher.state() == seq66::launcher_state::running, "running");

    bool ok = launcher.shutdown();
    CHECK(ok, "shutdown returns true");
    CHECK(launcher.state() == seq66::launcher_state::idle, "idle after shutdown");
    CHECK(launcher.generation() == 0, "generation reset to 0");
    CHECK(! launcher.engine_alive(), "engine not alive");
    CHECK(adapter->reserved_ports.empty(), "port released");
}

static void
test_shutdown_when_idle ()
{
    std::cout << "\n--- Shutdown when idle ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    seq66::sooperlooper_engine_launcher launcher(adapter, proc);

    bool ok = launcher.shutdown();
    CHECK(ok, "shutdown on idle returns true");
}

static void
test_deterministic_naming ()
{
    std::cout << "\n--- Deterministic naming ---" << std::endl;
    seq66::engine_naming_policy np;
    np.instance_id = "my-instance-abc";
    np.osc_port_base = 9951;
    np.osc_port_range = 100;
    np.jack_client_prefix = "seq66-sl";

    int port1 = np.compute_osc_port(1);
    int port2 = np.compute_osc_port(2);
    int port1_again = np.compute_osc_port(1);

    CHECK(port1 >= 9951, "port1 in range");
    CHECK(port1 < 10051, "port1 in range");
    CHECK(port2 >= 9951, "port2 in range");
    CHECK(port2 < 10051, "port2 in range");
    CHECK(port1 != port2, "different generations get different ports");
    CHECK(port1 == port1_again, "same generation is deterministic");

    std::string name1 = np.compute_jack_client_name(1);
    std::string name2 = np.compute_jack_client_name(2);
    std::string name1_again = np.compute_jack_client_name(1);

    CHECK(name1.find("seq66-sl") != std::string::npos,
          "name has prefix");
    CHECK(name1.find("g1") != std::string::npos, "name includes generation");
    CHECK(name2.find("g2") != std::string::npos,
          "different gen has different name");
    CHECK(name1 == name1_again, "same generation name is deterministic");
    CHECK(name1.size() <= 63, "name within JACK limit");
}

static void
test_naming_different_instances ()
{
    std::cout << "\n--- Different instances get different names ---" << std::endl;
    seq66::engine_naming_policy np1;
    np1.instance_id = "instance-A";
    np1.osc_port_base = 9951;
    np1.osc_port_range = 100;

    seq66::engine_naming_policy np2;
    np2.instance_id = "instance-B";
    np2.osc_port_base = 9951;
    np2.osc_port_range = 100;

    CHECK(np1.compute_osc_port(1) != np2.compute_osc_port(1),
          "different instances, different ports");
    CHECK(np1.compute_jack_client_name(1) != np2.compute_jack_client_name(1),
          "different instances, different names");
}

static void
test_port_conflict ()
{
    std::cout << "\n--- Port conflict prevents launch ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    adapter->probe_result = make_usable_probe();
    adapter->port_available = false;

    seq66::sooperlooper_engine_launcher launcher(adapter, proc);

    seq66::engine_naming_policy np;
    np.instance_id = "test";
    launcher.set_naming_policy(np);

    seq66::supervisor_config scfg;
    scfg.executable_path = "/usr/bin/sooperlooper";
    launcher.set_supervisor_config(scfg);

    auto result = launcher.launch();
    CHECK(result == seq66::launcher_result::naming_conflict,
          "returns naming_conflict");
    CHECK(proc->spawn_count == 0, "no process spawned");
}

static void
test_generation_sync_with_cache ()
{
    std::cout << "\n--- Generation syncs with observed cache ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    adapter->probe_result = make_usable_probe();

    seq66::sooperlooper_observed_cache cache;
    seq66::sooperlooper_engine_monitor monitor;
    seq66::sooperlooper_engine_launcher launcher(adapter, proc);
    launcher.set_observed_cache(&cache);
    launcher.set_engine_monitor(&monitor);

    seq66::engine_naming_policy np;
    np.instance_id = "test";
    launcher.set_naming_policy(np);

    seq66::supervisor_config scfg;
    scfg.executable_path = "/usr/bin/sooperlooper";
    launcher.set_supervisor_config(scfg);

    launcher.launch();

    CHECK(cache.generation() == 1,
          "cache generation matches launcher generation");
    CHECK(monitor.state() == seq66::engine_state::starting,
          "monitor reset to starting");
}

static void
test_stale_callback_rejection ()
{
    std::cout << "\n--- Stale callbacks rejected after relaunch ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    adapter->probe_result = make_usable_probe();

    seq66::sooperlooper_observed_cache cache;
    seq66::sooperlooper_engine_launcher launcher(adapter, proc);
    launcher.set_observed_cache(&cache);

    seq66::engine_naming_policy np;
    np.instance_id = "test";
    launcher.set_naming_policy(np);

    seq66::supervisor_config scfg;
    scfg.executable_path = "/usr/bin/sooperlooper";
    launcher.set_supervisor_config(scfg);

    /* First launch: generation 1 */
    launcher.launch();
    CHECK(cache.generation() == 1, "cache gen 1");

    /* Shutdown and relaunch: generation 2 */
    launcher.shutdown();
    launcher.launch();
    CHECK(cache.generation() == 2, "cache gen 2 after relaunch");

    /* An event from generation 1 should be rejected by the cache */
    std::vector<std::string> args = {"0", "state", "3"};
    bool applied = cache.apply("/sl/0/get", "isf", args, 1000000, 1);
    CHECK(! applied, "old generation event rejected");
}

static void
test_probe_error ()
{
    std::cout << "\n--- Probe error ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    adapter->probe_result.capability = seq66::backend_capability::probe_error;
    adapter->probe_result.error = "dlopen failed";

    seq66::sooperlooper_engine_launcher launcher(adapter, proc);

    auto result = launcher.launch();
    CHECK(result == seq66::launcher_result::probe_error,
          "returns probe_error");
    CHECK(launcher.state() == seq66::launcher_state::failed,
          "state is failed");
}

static void
test_no_adapter ()
{
    std::cout << "\n--- No adapter ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_engine_launcher launcher(nullptr, proc);

    auto result = launcher.launch();
    CHECK(result == seq66::launcher_result::probe_error,
          "returns probe_error without adapter");
}

static void
test_launcher_result_enum ()
{
    std::cout << "\n--- Launcher result enum ---" << std::endl;
    CHECK(static_cast<int>(seq66::launcher_result::success) == 0,
          "success is 0");
    CHECK(static_cast<int>(seq66::launcher_result::backend_unavailable) == 1,
          "backend_unavailable is 1");
    CHECK(static_cast<int>(seq66::launcher_result::probe_error) == 2,
          "probe_error is 2");
    CHECK(static_cast<int>(seq66::launcher_result::naming_conflict) == 3,
          "naming_conflict is 3");
    CHECK(static_cast<int>(seq66::launcher_result::supervisor_failed) == 4,
          "supervisor_failed is 4");
    CHECK(static_cast<int>(seq66::launcher_result::monitor_error) == 5,
          "monitor_error is 5");
}

static void
test_full_lifecycle ()
{
    std::cout << "\n--- Full lifecycle: launch → run → shutdown → relaunch ---"
              << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    adapter->probe_result = make_usable_probe();

    seq66::sooperlooper_observed_cache cache;
    seq66::sooperlooper_engine_launcher launcher(adapter, proc);
    launcher.set_observed_cache(&cache);

    seq66::engine_naming_policy np;
    np.instance_id = "lifecycle-test";
    launcher.set_naming_policy(np);

    seq66::supervisor_config scfg;
    scfg.executable_path = "/usr/bin/sooperlooper";
    launcher.set_supervisor_config(scfg);

    /* Launch */
    auto r1 = launcher.launch();
    CHECK(r1 == seq66::launcher_result::success, "launch 1 success");
    CHECK(launcher.generation() == 1, "gen 1");
    CHECK(cache.generation() == 1, "cache gen 1");

    /* Shutdown */
    launcher.shutdown();
    CHECK(launcher.generation() == 0, "gen reset to 0");

    /* Relaunch */
    auto r2 = launcher.launch();
    CHECK(r2 == seq66::launcher_result::success, "launch 2 success");
    CHECK(launcher.generation() == 2, "gen 2");
    CHECK(cache.generation() == 2, "cache gen 2");
}

static void
test_config_immutable_while_running ()
{
    std::cout << "\n--- Config immutable while running ---" << std::endl;
    auto proc = std::make_shared<fake_process_adapter>();
    auto adapter = std::make_shared<fake_launcher_adapter>();
    adapter->probe_result = make_usable_probe();

    seq66::sooperlooper_engine_launcher launcher(adapter, proc);

    seq66::engine_naming_policy np1;
    np1.instance_id = "original";
    launcher.set_naming_policy(np1);

    seq66::supervisor_config scfg;
    scfg.executable_path = "/usr/bin/sooperlooper";
    launcher.set_supervisor_config(scfg);

    launcher.launch();

    seq66::engine_naming_policy np2;
    np2.instance_id = "changed";
    launcher.set_naming_policy(np2);

    CHECK(launcher.naming_policy().instance_id == "original",
          "naming policy unchanged while running");
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int
main ()
{
    std::cout << "=== Engine launcher tests ===" << std::endl;

    test_initial_state();
    test_launcher_result_enum();
    test_deterministic_naming();
    test_naming_different_instances();
    test_launch_backend_unavailable();
    test_launch_success();
    test_launch_already_running();
    test_shutdown();
    test_shutdown_when_idle();
    test_port_conflict();
    test_generation_sync_with_cache();
    test_stale_callback_rejection();
    test_probe_error();
    test_no_adapter();
    test_full_lifecycle();
    test_config_immutable_while_running();

    std::cout << std::endl;
    if (s_failures == 0)
    {
        std::cout << "All engine launcher tests passed." << std::endl;
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
