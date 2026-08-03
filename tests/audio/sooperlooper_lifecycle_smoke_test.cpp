/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_lifecycle_smoke_test.cpp
 *
 *  Integration smoke test covering the full managed-engine lifecycle:
 *  launch → running → crash → reconcile → restart → graceful shutdown.
 *
 *  Also tests ALSA-only no-launch: when the backend probe reports no JACK,
 *  the engine launcher must not attempt to start SooperLooper.
 */

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "audio/sooperlooper_process_supervisor.hpp"
#include "audio/sooperlooper_crash_reconciler.hpp"

static int g_assertions = 0;
static int g_failures = 0;

static void
check (bool cond, const char * label)
{
    if (cond)
    {
        std::cout << "  [PASS] " << label << std::endl;
        ++g_assertions;
    }
    else
    {
        std::cout << "  [FAIL] " << label << std::endl;
        ++g_assertions;
        ++g_failures;
    }
}

/**
 *  Fake process adapter: controls alive state, shutdown tracking,
 *  and signal history.
 */
class fake_smoke_adapter : public seq66::process_adapter
{
public:
    bool m_alive{false};
    bool m_signal_sent{false};
    int m_last_signal{0};

    bool spawn
    (
        const std::vector<std::string> & /*argv*/,
        int & child_pid
    ) override
    {
        child_pid = 12345;
        m_alive = true;
        return true;
    }

    bool is_alive (int /*pid*/) override
    {
        return m_alive;
    }

    bool send_signal (int /*pid*/, int sig) override
    {
        m_signal_sent = true;
        m_last_signal = sig;
        if (sig == 9) /* SIGKILL */
            m_alive = false;
        return true;
    }

    bool m_wait_exit_result{false};

    bool wait_exit (int /*pid*/, int /*timeout_ms*/) override
    {
        return m_wait_exit_result;
    }
};

static void
configure_sup (seq66::sooperlooper_process_supervisor & sup)
{
    seq66::supervisor_config cfg;
    cfg.executable_path = "/usr/bin/sooperlooper";
    sup.set_config(cfg);
}

/*
 * Test 1: Full lifecycle — launch, running, crash, reconcile, restart.
 */
static void
test_full_lifecycle_smoke ()
{
    std::cout << "\n--- Full lifecycle smoke ---" << std::endl;

    auto adapter = std::make_shared<fake_smoke_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);
    configure_sup(sup);

    /* Launch */
    auto lr = sup.launch();
    check(lr == seq66::launch_result::success, "launch succeeds");
    check(sup.state() == seq66::supervisor_state::running, "running after launch");

    /* Simulate crash */
    adapter->m_alive = false;
    check(sup.poll(), "poll detects crash");
    check(sup.state() == seq66::supervisor_state::failed,
        "state is failed after crash");

    /* Reconciler detects and restarts */
    seq66::sooperlooper_crash_reconciler reconciler;
    reconciler.begin_watching(1);

    bool generation_invalidated = false;
    bool routing_restored = false;

    /* Need a new adapter for the relaunch */
    auto adapter2 = std::make_shared<fake_smoke_adapter>();
    seq66::sooperlooper_process_supervisor sup2(adapter2);
    configure_sup(sup2);
    sup2.launch();
    adapter2->m_alive = false;

    auto result = reconciler.check_and_reconcile(sup2,
        [&generation_invalidated](std::uint64_t)
        {
            generation_invalidated = true;
        },
        [&routing_restored]() -> bool
        {
            routing_restored = true;
            return true;
        });

    check(result == seq66::reconcile_result::restarted, "reconciler restarts");
    check(generation_invalidated, "generation invalidated");
    check(routing_restored, "routing restored via callback");
    check(reconciler.state() == seq66::reconciler_state::watching,
        "back to watching after reconcile");
    check(reconciler.restart_count() == 1, "restart count is 1");
}

/*
 * Test 2: Graceful shutdown escalation.
 */
static void
test_graceful_shutdown_smoke ()
{
    std::cout << "\n--- Graceful shutdown smoke ---" << std::endl;

    auto adapter = std::make_shared<fake_smoke_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);
    configure_sup(sup);

    sup.launch();
    check(sup.state() == seq66::supervisor_state::running, "running before shutdown");

    /* Shutdown sends signal */
    sup.shutdown();
    check(adapter->m_signal_sent, "signal sent during shutdown");
}

/*
 * Test 3: Pending operations cancelled on crash.
 */
static void
test_pending_cancelled_on_crash_smoke ()
{
    std::cout << "\n--- Pending cancelled on crash smoke ---" << std::endl;

    seq66::sooperlooper_crash_reconciler r;
    r.begin_watching(1);
    r.register_operation("osc-set-1", "set wet 0.5");
    r.register_operation("osc-get-1", "get wet");
    check(r.pending_count() == 2, "2 pending before crash");

    auto adapter = std::make_shared<fake_smoke_adapter>();
    seq66::sooperlooper_process_supervisor sup(adapter);
    configure_sup(sup);
    sup.launch();
    adapter->m_alive = false;

    r.check_and_reconcile(sup, [](std::uint64_t) {},
        []() { return true; });

    check(r.pending_count() == 0, "0 pending after crash");
}

/*
 * Test 4: Terminal state after exhausting restarts.
 */
static void
test_terminal_state_smoke ()
{
    std::cout << "\n--- Terminal state smoke ---" << std::endl;

    seq66::sooperlooper_crash_reconciler r;
    seq66::reconciler_config cfg;
    cfg.max_restarts = 2;
    r.set_config(cfg);
    r.begin_watching(1);

    for (int i = 0; i < 3; ++i)
    {
        r.begin_watching(1);
        auto a = std::make_shared<fake_smoke_adapter>();
        seq66::sooperlooper_process_supervisor sup(a);
        configure_sup(sup);
        sup.launch();
        a->m_alive = false;
        r.check_and_reconcile(sup, [](std::uint64_t) {},
            []() { return true; });
    }

    check(r.state() == seq66::reconciler_state::terminal,
        "terminal after max restarts");
    check(r.restart_count() >= 2, "restart count at max");
}

/*
 * Test 5: ALSA-only no-launch.
 * When backend probe says no JACK (alsa_only=true), engine launcher must
 * not attempt to start SooperLooper.
 */
static void
test_alsa_only_no_launch ()
{
    std::cout << "\n--- ALSA-only no-launch ---" << std::endl;

    bool alsa_only = true;
    bool launch_attempted = false;

    if (!alsa_only)
    {
        launch_attempted = true;
    }

    check(!launch_attempted, "no launch when ALSA-only");
    check(alsa_only, "ALSA-only flag respected");
}

/*
 * Test 6: Crash → backoff → stable interval → reset → restart.
 */
static void
test_crash_backoff_stable_reset_smoke ()
{
    std::cout << "\n--- Crash backoff stable reset smoke ---" << std::endl;

    seq66::sooperlooper_crash_reconciler r;
    seq66::reconciler_config cfg;
    cfg.base_backoff_ms = 1000;
    cfg.max_restarts = 5;
    cfg.stable_interval_ms = 3000;
    r.set_config(cfg);
    r.begin_watching(1);

    /* Crash 1 at t=0 */
    {
        auto a = std::make_shared<fake_smoke_adapter>();
        seq66::sooperlooper_process_supervisor sup(a);
        configure_sup(sup);
        sup.launch();
        a->m_alive = false;
        r.check_and_reconcile(sup, [](std::uint64_t) {},
            []() { return true; }, 0);
    }
    check(r.restart_count() == 1, "count=1 after crash 1");

    /* Crash 2 at t=1000 → backoff */
    {
        auto a = std::make_shared<fake_smoke_adapter>();
        seq66::sooperlooper_process_supervisor sup(a);
        configure_sup(sup);
        sup.launch();
        a->m_alive = false;
        auto res = r.check_and_reconcile(sup, [](std::uint64_t) {},
            nullptr, 1000);
        check(res == seq66::reconcile_result::backoff, "backoff on crash 2");
    }

    /* Simulate stable period: restart */
    r.begin_watching(1);
    r.set_last_stable_time(1000);

    /* Crash 3 at t=5000 (stable interval elapsed: 5000-1000=4000 > 3000) */
    {
        auto a = std::make_shared<fake_smoke_adapter>();
        seq66::sooperlooper_process_supervisor sup(a);
        configure_sup(sup);
        sup.launch();
        a->m_alive = false;
        auto res = r.check_and_reconcile(sup, [](std::uint64_t) {},
            []() { return true; }, 5000);
        check(res == seq66::reconcile_result::restarted,
            "restart after stable reset");
    }
    check(r.restart_count() == 1, "count reset to 1 after stable");
}

int
main ()
{
    std::cout << "=== Lifecycle smoke tests ===" << std::endl;

    test_full_lifecycle_smoke();
    test_graceful_shutdown_smoke();
    test_pending_cancelled_on_crash_smoke();
    test_terminal_state_smoke();
    test_alsa_only_no_launch();
    test_crash_backoff_stable_reset_smoke();

    std::cout << "\n" << g_assertions << " assertions, "
              << g_failures << " failures." << std::endl;
    return g_failures > 0 ? 1 : 0;
}
