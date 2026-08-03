/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_crash_reconciler_test.cpp
 *
 *  Tests for the crash detector and reconciler.
 *
 *  Pattern: launch with adapter alive, then set alive=false before poll()
 *  to simulate a crash that occurs after the process was running.
 */

#include "audio/sooperlooper_crash_reconciler.hpp"
#include "audio/sooperlooper_process_supervisor.hpp"

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

/*
 * Fake process adapter.
 */
class fake_process_adapter : public seq66::process_adapter
{
public:
    bool m_alive{true};
    bool m_wait_result{true};

    bool spawn (const std::vector<std::string> &, int & child_pid) override
    {
        child_pid = 42;
        return true;
    }
    bool send_signal (int, int) override { return true; }
    bool is_alive (int) override { return m_alive; }
    bool wait_exit (int, int) override { return m_wait_result; }
};

/*
 * Helper: configure supervisor with an executable path.
 */
static void
configure_sup (seq66::sooperlooper_process_supervisor & sup)
{
    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);
}

/* ---- Tests ---- */

static void
test_initial_state ()
{
    std::cout << "\n--- Initial state ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    CHECK(r.state() == seq66::reconciler_state::idle, "idle initially");
    CHECK(r.restart_count() == 0, "restart count 0");
    CHECK(r.watched_generation() == 0, "generation 0");
    CHECK(r.pending_count() == 0, "no pending ops");
}

static void
test_begin_watching ()
{
    std::cout << "\n--- Begin watching ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    r.begin_watching(42);
    CHECK(r.state() == seq66::reconciler_state::watching, "watching");
    CHECK(r.watched_generation() == 42, "generation 42");
}

static void
test_stop_watching ()
{
    std::cout << "\n--- Stop watching ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    r.begin_watching(1);
    r.register_operation("op1", "test");
    r.stop_watching();
    CHECK(r.state() == seq66::reconciler_state::idle, "idle after stop");
    CHECK(r.pending_count() == 0, "pending cleared");
}

static void
test_register_complete_operation ()
{
    std::cout << "\n--- Register/complete operation ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    r.begin_watching(1);

    CHECK(r.register_operation("op1", "subscribe"), "register op1");
    CHECK(r.pending_count() == 1, "1 pending");
    CHECK(! r.register_operation("op1", "dup"), "duplicate rejected");
    CHECK(r.complete_operation("op1"), "complete op1");
    CHECK(! r.complete_operation("op1"), "already completed");

    const auto & ops = r.pending_operations();
    CHECK(ops.size() == 1, "1 in list");
    CHECK(ops[0].completed, "op1 completed");
    CHECK(ops[0].generation == 1, "gen 1");
}

static void
test_cancel_all_pending ()
{
    std::cout << "\n--- Cancel all pending ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    r.begin_watching(1);
    r.register_operation("op1", "sub1");
    r.register_operation("op2", "sub2");
    CHECK(r.pending_count() == 2, "2 pending");
    r.cancel_all_pending();
    CHECK(r.pending_count() == 0, "0 pending after cancel");
}

static void
test_no_crash_when_not_watching ()
{
    std::cout << "\n--- No crash when not watching ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);
    configure_sup(sup);

    auto result = r.check_and_reconcile(sup, nullptr, nullptr);
    CHECK(result == seq66::reconcile_result::none, "none when idle");
}

static void
test_no_crash_when_alive ()
{
    std::cout << "\n--- No crash when child alive ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    r.begin_watching(1);

    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);
    configure_sup(sup);
    sup.launch();  /* state = running, m_alive = true */

    auto result = r.check_and_reconcile(sup, nullptr, nullptr);
    CHECK(result == seq66::reconcile_result::none, "none when alive");
    CHECK(r.state() == seq66::reconciler_state::watching, "still watching");
}

static void
test_first_crash_immediate_restart ()
{
    std::cout << "\n--- First crash: immediate restart ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    r.begin_watching(1);

    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);
    configure_sup(sup);
    sup.launch();  /* running */
    adapter->m_alive = false;  /* simulate crash */

    bool restart_called = false;
    auto result = r.check_and_reconcile(sup,
        [](std::uint64_t) {},
        [&restart_called]() { restart_called = true; return true; });

    CHECK(result == seq66::reconcile_result::restarted, "restarted");
    CHECK(restart_called, "restart callback called");
    CHECK(r.restart_count() == 1, "restart count 1");
    CHECK(r.state() == seq66::reconciler_state::watching, "back to watching");
}

static void
test_crash_backoff ()
{
    std::cout << "\n--- Crash with backoff ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    seq66::reconciler_config cfg;
    cfg.base_backoff_ms = 1000;
    r.set_config(cfg);
    r.begin_watching(1);

    /* First crash: immediate restart (backoff=0 at count=0) */
    {
        auto adapter = std::make_shared<fake_process_adapter>();
        seq66::sooperlooper_process_supervisor sup(adapter);
        configure_sup(sup);
        sup.launch();
        adapter->m_alive = false;
        r.check_and_reconcile(sup, [](std::uint64_t) {},
            []() { return true; });
    }
    CHECK(r.restart_count() == 1, "restart count 1");

    /* Second crash: backoff applies */
    {
        auto adapter = std::make_shared<fake_process_adapter>();
        seq66::sooperlooper_process_supervisor sup(adapter);
        configure_sup(sup);
        sup.launch();
        adapter->m_alive = false;
        auto result = r.check_and_reconcile(sup,
            [](std::uint64_t) {}, nullptr);
        CHECK(result == seq66::reconcile_result::backoff,
            "backoff on 2nd crash");
    }
    CHECK(r.state() == seq66::reconciler_state::backoff, "state is backoff");
    CHECK(r.current_backoff_ms() == 2000, "backoff 2000ms at count 2");
}

static void
test_terminal_after_max_restarts ()
{
    std::cout << "\n--- Terminal after max restarts ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    seq66::reconciler_config cfg;
    cfg.max_restarts = 2;
    cfg.base_backoff_ms = 0;
    r.set_config(cfg);
    r.begin_watching(1);

    /* Crash 1: restart */
    {
        auto a = std::make_shared<fake_process_adapter>();
        seq66::sooperlooper_process_supervisor sup(a);
        configure_sup(sup);
        sup.launch();
        a->m_alive = false;
        r.check_and_reconcile(sup, [](std::uint64_t) {},
            []() { return true; });
    }
    CHECK(r.restart_count() == 1, "restart 1");

    /* Crash 2: restart */
    {
        auto a = std::make_shared<fake_process_adapter>();
        seq66::sooperlooper_process_supervisor sup(a);
        configure_sup(sup);
        sup.launch();
        a->m_alive = false;
        r.check_and_reconcile(sup, [](std::uint64_t) {},
            []() { return true; });
    }
    CHECK(r.restart_count() == 2, "restart 2");

    /* Crash 3: terminal (max_restarts=2 exhausted) */
    {
        auto a = std::make_shared<fake_process_adapter>();
        seq66::sooperlooper_process_supervisor sup(a);
        configure_sup(sup);
        sup.launch();
        a->m_alive = false;
        auto result = r.check_and_reconcile(sup,
            [](std::uint64_t) {}, nullptr);
        CHECK(result == seq66::reconcile_result::terminal, "terminal");
    }
    CHECK(r.state() == seq66::reconciler_state::terminal, "state terminal");
}

static void
test_generation_invalidated ()
{
    std::cout << "\n--- Generation invalidated on crash ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    r.begin_watching(7);

    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);
    configure_sup(sup);
    sup.launch();
    adapter->m_alive = false;

    std::uint64_t invalidated_gen = 0;
    r.check_and_reconcile(sup,
        [&invalidated_gen](std::uint64_t gen) { invalidated_gen = gen; },
        []() { return true; });
    CHECK(invalidated_gen == 7, "generation 7 invalidated");
}

static void
test_pending_cancelled_on_crash ()
{
    std::cout << "\n--- Pending cancelled on crash ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    r.begin_watching(1);
    r.register_operation("op1", "sub1");
    r.register_operation("op2", "sub2");
    CHECK(r.pending_count() == 2, "2 pending before crash");

    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);
    configure_sup(sup);
    sup.launch();
    adapter->m_alive = false;

    r.check_and_reconcile(sup, [](std::uint64_t) {},
        []() { return true; });
    CHECK(r.pending_count() == 0, "0 pending after crash");
}

static void
test_shutdown_during_startup ()
{
    std::cout << "\n--- Shutdown during startup ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    r.begin_watching(1);

    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    bool handled = r.handle_shutdown_during_startup(sup);
    CHECK(handled, "shutdown handled");
    CHECK(r.state() == seq66::reconciler_state::idle, "idle after");
}

static void
test_shutdown_not_during_startup ()
{
    std::cout << "\n--- Shutdown not during startup ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    r.force_state(seq66::reconciler_state::backoff);

    auto adapter = std::make_shared<fake_process_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);

    bool handled = r.handle_shutdown_during_startup(sup);
    CHECK(! handled, "not handled when not watching");
}

static void
test_backoff_computation ()
{
    std::cout << "\n--- Backoff computation ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    seq66::reconciler_config cfg;
    cfg.base_backoff_ms = 1000;
    cfg.backoff_multiplier = 2.0;
    cfg.max_backoff_ms = 30000;
    r.set_config(cfg);

    r.force_restart_count(0);
    CHECK(r.current_backoff_ms() == 0, "count 0: 0ms");
    r.force_restart_count(1);
    CHECK(r.current_backoff_ms() == 1000, "count 1: 1000ms");
    r.force_restart_count(2);
    CHECK(r.current_backoff_ms() == 2000, "count 2: 2000ms");
    r.force_restart_count(3);
    CHECK(r.current_backoff_ms() == 4000, "count 3: 4000ms");
    r.force_restart_count(10);
    CHECK(r.current_backoff_ms() == 30000, "count 10: capped 30000ms");
}

static void
test_backoff_elapsed ()
{
    std::cout << "\n--- Backoff elapsed ---" << std::endl;
    seq66::sooperlooper_crash_reconciler r;
    seq66::reconciler_config cfg;
    cfg.base_backoff_ms = 1000;
    r.set_config(cfg);
    r.force_state(seq66::reconciler_state::backoff);
    r.record_crash_time(10000);
    r.force_restart_count(2);

    CHECK(! r.backoff_elapsed(10500), "not elapsed at 500ms");
    CHECK(r.backoff_elapsed(11000), "elapsed at 1000ms");
    CHECK(r.backoff_elapsed(12000), "elapsed at 2000ms");
}

static void
test_reconciler_state_enum ()
{
    std::cout << "\n--- Reconciler state enum ---" << std::endl;
    CHECK(static_cast<int>(seq66::reconciler_state::idle) == 0, "idle 0");
    CHECK(static_cast<int>(seq66::reconciler_state::watching) == 1, "watching 1");
    CHECK(static_cast<int>(seq66::reconciler_state::reconciling) == 2, "reconciling 2");
    CHECK(static_cast<int>(seq66::reconciler_state::backoff) == 3, "backoff 3");
    CHECK(static_cast<int>(seq66::reconciler_state::terminal) == 4, "terminal 4");
}

int
main ()
{
    std::cout << "=== Crash reconciler tests ===" << std::endl;

    test_initial_state();
    test_begin_watching();
    test_stop_watching();
    test_register_complete_operation();
    test_cancel_all_pending();
    test_no_crash_when_not_watching();
    test_no_crash_when_alive();
    test_first_crash_immediate_restart();
    test_crash_backoff();
    test_terminal_after_max_restarts();
    test_generation_invalidated();
    test_pending_cancelled_on_crash();
    test_shutdown_during_startup();
    test_shutdown_not_during_startup();
    test_backoff_computation();
    test_backoff_elapsed();
    test_reconciler_state_enum();

    std::cout << std::endl;
    if (s_failures == 0)
    {
        std::cout << "All crash reconciler tests passed." << std::endl;
        return 0;
    }
    else
    {
        std::cerr << s_failures << " test(s) FAILED." << std::endl;
        return 1;
    }
}
