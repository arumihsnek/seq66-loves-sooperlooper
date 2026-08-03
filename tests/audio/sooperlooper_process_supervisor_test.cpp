/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_process_supervisor_test.cpp
 *
 *  Tests for the managed process supervisor.  Uses a fake process adapter
 *  to verify lifecycle, identity verification, shutdown escalation,
 *  generation tracking and error handling without forking real processes.
 *
 *  Build command (mirrors audio-core.yml):
 *      g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -pthread \
 *          -Ilibseq66/include \
 *          libseq66/src/audio/sooperlooper_process_supervisor.cpp \
 *          tests/audio/sooperlooper_process_supervisor_test.cpp \
 *          -o .ci/bin/sooperlooper_process_supervisor_test
 */

#include "audio/sooperlooper_process_supervisor.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <iostream>
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
    /* Configuration */
    bool spawn_succeeds{true};
    bool alive_after_spawn{true};
    bool signal_succeeds{true};
    bool wait_exits{true};
    bool sigkill_always_works{true};  /* SIGKILL is unblockable */
    int next_pid{42};

    /* Recording */
    int last_spawn_pid{0};
    std::vector<std::string> last_spawn_argv;
    int last_signal_pid{0};
    int last_signal_num{0};
    int spawn_count{0};
    int signal_count{0};
    int wait_count{0};
    std::vector<int> wait_pids;

    bool spawn
    (
        const std::vector<std::string> & argv,
        int & child_pid
    ) override
    {
        ++spawn_count;
        last_spawn_argv = argv;
        if (spawn_succeeds)
        {
            child_pid = next_pid;
            last_spawn_pid = child_pid;
            return true;
        }
        return false;
    }

    bool send_signal (int pid, int signal) override
    {
        ++signal_count;
        last_signal_pid = pid;
        last_signal_num = signal;
        return signal_succeeds;
    }

    bool is_alive (int pid) override
    {
        /* After spawn, the child is alive until explicitly killed */
        if (pid == last_spawn_pid && alive_after_spawn)
            return true;
        return false;
    }

    bool wait_exit (int pid, int /* timeout_ms */) override
    {
        ++wait_count;
        wait_pids.push_back(pid);
        /* SIGKILL (signal 9) is unblockable — always succeeds */
        if (sigkill_always_works && last_signal_num == 9)
            return true;
        return wait_exits;
    }
};

/* ------------------------------------------------------------------ */
/*  Test groups                                                        */
/* ------------------------------------------------------------------ */

static void
test_initial_state ()
{
    std::cout << "\n--- Initial state ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    CHECK(sup.state() == seq66::supervisor_state::idle,
          "initial state is idle");
    CHECK(sup.generation() == 0, "initial generation is 0");
    CHECK(sup.child_pid() == 0, "initial child_pid is 0");
    CHECK(! sup.child_alive(), "child_alive is false initially");
    CHECK(sup.restart_count() == 0, "restart_count is 0 initially");
    CHECK(sup.last_error().empty(), "no error initially");
}

static void
test_successful_launch ()
{
    std::cout << "\n--- Successful launch ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    cfg.args = {"--headless"};
    sup.set_config(cfg);

    auto result = sup.launch();
    CHECK(result == seq66::launch_result::success, "launch returns success");
    CHECK(sup.state() == seq66::supervisor_state::running,
          "state is running after launch");
    CHECK(sup.generation() == 1, "generation is 1 after first launch");
    CHECK(sup.child_pid() == 42, "child_pid is 42 (fake)");
    CHECK(sup.child_alive(), "child_alive returns true");
    CHECK(adapter->spawn_count == 1, "spawn called once");
    CHECK(adapter->last_spawn_argv.size() == 2, "argv has 2 elements");
    CHECK(adapter->last_spawn_argv[0] == "/usr/bin/sooperlooper",
          "argv[0] is executable path");
    CHECK(adapter->last_spawn_argv[1] == "--headless",
          "argv[1] is --headless");
}

static void
test_launch_already_running ()
{
    std::cout << "\n--- Launch when already running ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);

    sup.launch();
    auto result = sup.launch();
    CHECK(result == seq66::launch_result::already_running,
          "second launch returns already_running");
    CHECK(sup.state() == seq66::supervisor_state::running,
          "state remains running");
    CHECK(sup.generation() == 1, "generation not incremented");
    CHECK(adapter->spawn_count == 1, "spawn called only once");
}

static void
test_launch_fork_failed ()
{
    std::cout << "\n--- Launch fork failure ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    adapter->spawn_succeeds = false;
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);

    auto result = sup.launch();
    CHECK(result == seq66::launch_result::fork_failed,
          "launch returns fork_failed");
    CHECK(sup.state() == seq66::supervisor_state::failed,
          "state is failed");
    CHECK(! sup.last_error().empty(), "error message is set");
}

static void
test_launch_no_executable ()
{
    std::cout << "\n--- Launch without executable path ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    /* Don't set config — empty executable path */
    auto result = sup.launch();
    CHECK(result == seq66::launch_result::exec_failed,
          "launch returns exec_failed");
    CHECK(sup.state() == seq66::supervisor_state::failed,
          "state is failed");
}

static void
test_launch_no_adapter ()
{
    std::cout << "\n--- Launch without adapter ---" << std::endl;
    seq66::sooperlooper_process_supervisor sup;

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);

    auto result = sup.launch();
    CHECK(result == seq66::launch_result::fork_failed,
          "launch returns fork_failed without adapter");
}

static void
test_launch_identity_mismatch ()
{
    std::cout << "\n--- Launch identity mismatch ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    adapter->alive_after_spawn = false;  /* PID check fails */
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);

    auto result = sup.launch();
    CHECK(result == seq66::launch_result::identity_mismatch,
          "launch returns identity_mismatch");
    CHECK(sup.state() == seq66::supervisor_state::failed,
          "state is failed");
}

static void
test_generation_increments ()
{
    std::cout << "\n--- Generation increments on each launch ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);

    sup.launch();
    CHECK(sup.generation() == 1, "generation is 1 after first launch");

    sup.shutdown();
    sup.launch();
    CHECK(sup.generation() == 2, "generation is 2 after second launch");

    sup.shutdown();
    sup.launch();
    CHECK(sup.generation() == 3, "generation is 3 after third launch");
}

static void
test_shutdown_graceful ()
{
    std::cout << "\n--- Graceful shutdown (child exits in time) ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    adapter->wait_exits = true;
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    cfg.graceful_timeout_ms = 2000;
    sup.set_config(cfg);

    sup.launch();
    CHECK(sup.state() == seq66::supervisor_state::running, "running");

    bool ok = sup.shutdown();
    CHECK(ok, "shutdown returns true");
    CHECK(sup.state() == seq66::supervisor_state::idle,
          "state is idle after shutdown");
    CHECK(sup.child_pid() == 0, "child_pid is 0 after shutdown");
    CHECK(! sup.child_alive(), "child_alive is false");
    CHECK(adapter->signal_count == 0, "no signals sent (graceful exit)");
    CHECK(adapter->wait_count == 1, "wait_exit called once");
}

static void
test_shutdown_escalation_to_term ()
{
    std::cout << "\n--- Shutdown escalation to SIGTERM ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    adapter->wait_exits = false;  /* graceful wait times out */
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    cfg.graceful_timeout_ms = 100;
    cfg.term_timeout_ms = 100;
    sup.set_config(cfg);

    sup.launch();
    bool ok = sup.shutdown();
    CHECK(ok, "shutdown returns true (SIGKILL unblockable)");
    CHECK(sup.state() == seq66::supervisor_state::idle,
          "state is idle after escalation");
    CHECK(adapter->signal_count >= 2, "at least two signals sent");
    CHECK(adapter->last_signal_num == 9, "final signal was SIGKILL");
}

static void
test_shutdown_escalation_to_kill ()
{
    std::cout << "\n--- Shutdown escalation to SIGKILL ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    adapter->wait_exits = false;  /* graceful and SIGTERM timeout */
    adapter->sigkill_always_works = false;  /* simulate SIGKILL failure */
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    cfg.graceful_timeout_ms = 10;
    cfg.term_timeout_ms = 10;
    sup.set_config(cfg);

    sup.launch();
    bool ok = sup.shutdown();
    CHECK(! ok, "shutdown returns false (SIGKILL failed)");
    CHECK(sup.state() == seq66::supervisor_state::failed,
          "state is failed when SIGKILL fails");
    CHECK(adapter->signal_count >= 2, "at least two signals sent");
    CHECK(adapter->last_signal_num == 9, "final signal was SIGKILL");
}

static void
test_shutdown_when_idle ()
{
    std::cout << "\n--- Shutdown when idle ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    bool ok = sup.shutdown();
    CHECK(ok, "shutdown on idle returns true");
    CHECK(sup.state() == seq66::supervisor_state::idle,
          "state remains idle");
}

static void
test_force_kill ()
{
    std::cout << "\n--- Force kill ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);

    sup.launch();
    CHECK(sup.state() == seq66::supervisor_state::running, "running");

    sup.force_kill();
    CHECK(sup.state() == seq66::supervisor_state::idle,
          "state is idle after force kill");
    CHECK(sup.child_pid() == 0, "child_pid is 0");
    CHECK(adapter->last_signal_num == 9, "SIGKILL was sent");
}

static void
test_restart ()
{
    std::cout << "\n--- Restart ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    cfg.max_restarts = 5;
    sup.set_config(cfg);

    sup.launch();
    CHECK(sup.generation() == 1, "generation 1");
    CHECK(sup.restart_count() == 0, "restart_count 0");

    auto result = sup.restart();
    CHECK(result == seq66::launch_result::success,
          "restart returns success");
    CHECK(sup.generation() == 2, "generation 2 after restart");
    CHECK(sup.restart_count() == 1, "restart_count 1");
    CHECK(sup.state() == seq66::supervisor_state::running,
          "state is running after restart");
}

static void
test_restart_max_limit ()
{
    std::cout << "\n--- Restart max limit ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    cfg.max_restarts = 2;
    sup.set_config(cfg);

    sup.launch();
    sup.restart();  /* restart_count = 1 */
    sup.restart();  /* restart_count = 2 */

    auto result = sup.restart();  /* should fail: restart_count = 2 >= max */
    CHECK(result == seq66::launch_result::fork_failed,
          "restart fails after max limit");
    CHECK(! sup.last_error().empty(), "error message set");
}

static void
test_restart_from_idle ()
{
    std::cout << "\n--- Restart from idle ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);

    auto result = sup.restart();
    CHECK(result == seq66::launch_result::success,
          "restart from idle launches successfully");
    CHECK(sup.state() == seq66::supervisor_state::running,
          "state is running");
}

static void
test_poll_running ()
{
    std::cout << "\n--- Poll while running (child alive) ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);

    sup.launch();
    bool changed = sup.poll();
    CHECK(! changed, "poll returns false when child is alive");
    CHECK(sup.state() == seq66::supervisor_state::running,
          "state remains running");
}

static void
test_poll_child_exited ()
{
    std::cout << "\n--- Poll after child exited ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    adapter->alive_after_spawn = false;
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);

    /* Launch with alive check, then simulate death */
    adapter->alive_after_spawn = true;
    sup.launch();
    adapter->alive_after_spawn = false;  /* child dies */

    bool changed = sup.poll();
    CHECK(changed, "poll returns true when child exited");
    CHECK(sup.state() == seq66::supervisor_state::failed,
          "state is failed after unexpected exit");
    CHECK(sup.child_pid() == 0, "child_pid is 0");
}

static void
test_poll_not_running ()
{
    std::cout << "\n--- Poll when not running ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    bool changed = sup.poll();
    CHECK(! changed, "poll returns false when idle");
}

static void
test_build_argv ()
{
    std::cout << "\n--- Build argv ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    cfg.args = {"--headless", "--loop-count", "0"};
    sup.set_config(cfg);

    auto argv = sup.build_argv();
    CHECK(argv.size() == 4, "argv has 4 elements");
    CHECK(argv[0] == "/usr/bin/sooperlooper", "argv[0] is executable");
    CHECK(argv[1] == "--headless", "argv[1] is --headless");
    CHECK(argv[2] == "--loop-count", "argv[2] is --loop-count");
    CHECK(argv[3] == "0", "argv[3] is 0");
}

static void
test_signals_only_to_owned_child ()
{
    std::cout << "\n--- Signals only sent to owned child ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);

    sup.launch();
    int owned_pid = sup.child_pid();
    CHECK(owned_pid == 42, "owned child PID is 42");

    sup.force_kill();
    CHECK(adapter->last_signal_pid == owned_pid,
          "signal sent to the owned child PID only");
}

static void
test_destructor_calls_shutdown ()
{
    std::cout << "\n--- Destructor calls shutdown ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();

    {
        seq66::sooperlooper_process_supervisor sup(adapter);
        seq66::supervisor_config cfg;
        cfg.executable_path = "/usr/bin/sooperlooper";
        sup.set_config(cfg);
        sup.launch();
        CHECK(sup.state() == seq66::supervisor_state::running, "running");
    }
    /* Destructor should have called shutdown */
    CHECK(adapter->wait_count >= 1 || adapter->signal_count >= 1,
          "shutdown was called during destruction");
}

static void
test_config_immutable_while_running ()
{
    std::cout << "\n--- Config immutable while running ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg1;
    cfg1.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg1);
    sup.launch();

    seq66::supervisor_config cfg2;
    cfg2.executable_path = "/usr/bin/changed";
    sup.set_config(cfg2);  /* Should not change while running */

    CHECK(sup.get_config().executable_path == "/usr/bin/sooperlooper",
          "config unchanged while running");
}

static void
test_config_changeable_when_idle ()
{
    std::cout << "\n--- Config changeable when idle ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg1;
    cfg1.executable_path = "/usr/bin/original";
    sup.set_config(cfg1);

    seq66::supervisor_config cfg2;
    cfg2.executable_path = "/usr/bin/updated";
    sup.set_config(cfg2);

    CHECK(sup.get_config().executable_path == "/usr/bin/updated",
          "config updated when idle");
}

static void
test_config_changeable_when_failed ()
{
    std::cout << "\n--- Config changeable when failed ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    adapter->spawn_succeeds = false;
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg1;
    cfg1.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg1);
    sup.launch();  /* Will fail */

    CHECK(sup.state() == seq66::supervisor_state::failed, "state is failed");

    seq66::supervisor_config cfg2;
    cfg2.executable_path = "/usr/bin/updated";
    sup.set_config(cfg2);

    CHECK(sup.get_config().executable_path == "/usr/bin/updated",
          "config updated when failed");
}

static void
test_state_enum_values ()
{
    std::cout << "\n--- Supervisor state enum ---" << std::endl;
    CHECK(static_cast<int>(seq66::supervisor_state::idle) == 0,
          "idle is 0");
    CHECK(static_cast<int>(seq66::supervisor_state::starting) == 1,
          "starting is 1");
    CHECK(static_cast<int>(seq66::supervisor_state::running) == 2,
          "running is 2");
    CHECK(static_cast<int>(seq66::supervisor_state::stopping) == 3,
          "stopping is 3");
    CHECK(static_cast<int>(seq66::supervisor_state::failed) == 4,
          "failed is 4");
    CHECK(static_cast<int>(seq66::supervisor_state::cooldown) == 5,
          "cooldown is 5");
}

static void
test_launch_result_enum ()
{
    std::cout << "\n--- Launch result enum ---" << std::endl;
    CHECK(static_cast<int>(seq66::launch_result::success) == 0,
          "success is 0");
    CHECK(static_cast<int>(seq66::launch_result::already_running) == 1,
          "already_running is 1");
    CHECK(static_cast<int>(seq66::launch_result::fork_failed) == 2,
          "fork_failed is 2");
    CHECK(static_cast<int>(seq66::launch_result::exec_failed) == 3,
          "exec_failed is 3");
    CHECK(static_cast<int>(seq66::launch_result::identity_mismatch) == 4,
          "identity_mismatch is 4");
    CHECK(static_cast<int>(seq66::launch_result::backend_blocked) == 5,
          "backend_blocked is 5");
}

static void
test_full_lifecycle ()
{
    std::cout << "\n--- Full lifecycle: launch → run → shutdown → relaunch ---"
              << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    cfg.args = {"--headless"};
    sup.set_config(cfg);

    /* Launch */
    auto r1 = sup.launch();
    CHECK(r1 == seq66::launch_result::success, "launch 1 success");
    CHECK(sup.generation() == 1, "gen 1");
    CHECK(sup.state() == seq66::supervisor_state::running, "running");

    /* Shutdown */
    sup.shutdown();
    CHECK(sup.state() == seq66::supervisor_state::idle, "idle");
    CHECK(sup.generation() == 1, "gen still 1 after shutdown");

    /* Relaunch */
    auto r2 = sup.launch();
    CHECK(r2 == seq66::launch_result::success, "launch 2 success");
    CHECK(sup.generation() == 2, "gen 2 after relaunch");
    CHECK(sup.state() == seq66::supervisor_state::running, "running again");
}

static void
test_force_state ()
{
    std::cout << "\n--- Force state ---" << std::endl;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    sup.force_state(seq66::supervisor_state::failed);
    CHECK(sup.state() == seq66::supervisor_state::failed,
          "force_state sets state");

    sup.force_state(seq66::supervisor_state::cooldown);
    CHECK(sup.state() == seq66::supervisor_state::cooldown,
          "force_state to cooldown");
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int
main ()
{
    std::cout << "=== Process supervisor tests ===" << std::endl;

    /* State queries */
    test_initial_state();
    test_state_enum_values();
    test_launch_result_enum();

    /* Launch */
    test_successful_launch();
    test_launch_already_running();
    test_launch_fork_failed();
    test_launch_no_executable();
    test_launch_no_adapter();
    test_launch_identity_mismatch();

    /* Generation */
    test_generation_increments();

    /* Shutdown */
    test_shutdown_graceful();
    test_shutdown_escalation_to_term();
    test_shutdown_escalation_to_kill();
    test_shutdown_when_idle();

    /* Force kill */
    test_force_kill();

    /* Restart */
    test_restart();
    test_restart_max_limit();
    test_restart_from_idle();

    /* Polling */
    test_poll_running();
    test_poll_child_exited();
    test_poll_not_running();

    /* Owned-child-only signaling */
    test_signals_only_to_owned_child();

    /* Build argv */
    test_build_argv();

    /* Destructor */
    test_destructor_calls_shutdown();

    /* Config immutability */
    test_config_immutable_while_running();
    test_config_changeable_when_idle();
    test_config_changeable_when_failed();

    /* Force state */
    test_force_state();

    /* Full lifecycle */
    test_full_lifecycle();

    std::cout << std::endl;
    if (s_failures == 0)
    {
        std::cout << "All process supervisor tests passed." << std::endl;
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
