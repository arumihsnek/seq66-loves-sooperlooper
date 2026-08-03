/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_engine_monitor_test.cpp
 *
 *  Unit tests for the engine lifecycle monitor.
 */

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "seq66-config.h"
#include "audio/sooperlooper_engine_monitor.hpp"

using namespace seq66;

int
main ()
{
    std::cout << "Testing sooperlooper_engine_monitor..." << std::endl;
    int failures = 0;

    // ---- Test 1: initial state is disabled ----
    {
        sooperlooper_engine_monitor mon;
        if (mon.state() != engine_state::disabled)
        {
            std::cerr << "ERROR: Initial state should be disabled." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Initial state is disabled." << std::endl;
    }

    // ---- Test 2: ping_sent transitions to starting ----
    {
        sooperlooper_engine_monitor mon;
        mon.ping_sent();
        if (mon.state() != engine_state::starting)
        {
            std::cerr << "ERROR: State should be starting after ping_sent." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] ping_sent transitions to starting." << std::endl;
    }

    // ---- Test 3: ping_reply transitions to reconciling ----
    {
        sooperlooper_engine_monitor mon;
        mon.ping_sent();
        mon.ping_reply("1.7.9", 4);
        if (mon.state() != engine_state::reconciling)
        {
            std::cerr << "ERROR: State should be reconciling after ping_reply." << std::endl;
            ++failures;
        }
        if (mon.version() != "1.7.9")
        {
            std::cerr << "ERROR: Version should be 1.7.9." << std::endl;
            ++failures;
        }
        if (mon.loop_count() != 4)
        {
            std::cerr << "ERROR: Loop count should be 4." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] ping_reply transitions to reconciling." << std::endl;
    }

    // ---- Test 4: force to ready, then stale after missed pings ----
    {
        sooperlooper_engine_monitor mon;
        sooperlooper_engine_monitor::config cfg;
        cfg.stale_threshold = 3;
        mon.set_config(cfg);
        mon.force_state(engine_state::ready);

        mon.ping_missed();
        mon.ping_missed();
        mon.evaluate();
        if (mon.state() != engine_state::ready)
        {
            std::cerr << "ERROR: Should still be ready after 2 missed pings." << std::endl;
            ++failures;
        }
        mon.ping_missed();
        mon.evaluate();
        if (mon.state() != engine_state::stale)
        {
            std::cerr << "ERROR: Should be stale after 3 missed pings." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Stale after threshold missed pings." << std::endl;
    }

    // ---- Test 5: stale -> engine_offline after continued misses ----
    {
        sooperlooper_engine_monitor mon;
        sooperlooper_engine_monitor::config cfg;
        cfg.stale_threshold = 2;
        mon.set_config(cfg);
        mon.force_state(engine_state::stale);
        // stale_threshold * 2 = 4 missed pings to go offline
        for (int i = 0; i < 4; ++i)
            mon.ping_missed();
        mon.evaluate();
        if (mon.state() != engine_state::engine_offline)
        {
            std::cerr << "ERROR: Should be engine_offline after extended stale." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Stale -> engine_offline." << std::endl;
    }

    // ---- Test 6: startup deadline -> engine_offline ----
    {
        sooperlooper_engine_monitor mon;
        sooperlooper_engine_monitor::config cfg;
        cfg.startup_deadline_ms = 1;  // 1ms deadline
        mon.set_config(cfg);
        mon.ping_sent();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        mon.evaluate();
        if (mon.state() != engine_state::engine_offline)
        {
            std::cerr << "ERROR: Should be engine_offline after startup deadline." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Startup deadline -> engine_offline." << std::endl;
    }

    // ---- Test 7: ping_reply recovers from stale ----
    {
        sooperlooper_engine_monitor mon;
        mon.force_state(engine_state::stale);
        mon.ping_reply("1.7.9", 2);
        if (mon.state() != engine_state::ready)
        {
            std::cerr << "ERROR: ping_reply from stale should go to ready." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] ping_reply recovers from stale." << std::endl;
    }

    // ---- Test 8: reset clears state ----
    {
        sooperlooper_engine_monitor mon;
        mon.ping_sent();
        mon.ping_reply("1.7.9", 4);
        mon.reset();
        if (mon.state() != engine_state::disabled)
        {
            std::cerr << "ERROR: State should be disabled after reset." << std::endl;
            ++failures;
        }
        if (!mon.version().empty())
        {
            std::cerr << "ERROR: Version should be empty after reset." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] reset clears state." << std::endl;
    }

    // ---- Test 9: is_ready and is_unreachable ----
    {
        sooperlooper_engine_monitor mon;
        if (mon.is_ready())
        {
            std::cerr << "ERROR: Should not be ready when disabled." << std::endl;
            ++failures;
        }
        mon.force_state(engine_state::ready);
        if (!mon.is_ready())
        {
            std::cerr << "ERROR: Should be ready." << std::endl;
            ++failures;
        }
        mon.force_state(engine_state::stale);
        if (!mon.is_unreachable())
        {
            std::cerr << "ERROR: Should be unreachable when stale." << std::endl;
            ++failures;
        }
        mon.force_state(engine_state::engine_offline);
        if (!mon.is_unreachable())
        {
            std::cerr << "ERROR: Should be unreachable when offline." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] is_ready and is_unreachable." << std::endl;
    }

    // ---- Test 10: config is stored and retrieved ----
    {
        sooperlooper_engine_monitor mon;
        sooperlooper_engine_monitor::config cfg;
        cfg.ping_interval_ms = 2000;
        cfg.stale_threshold = 5;
        mon.set_config(cfg);
        if (mon.get_config().ping_interval_ms != 2000)
        {
            std::cerr << "ERROR: ping_interval_ms should be 2000." << std::endl;
            ++failures;
        }
        if (mon.get_config().stale_threshold != 5)
        {
            std::cerr << "ERROR: stale_threshold should be 5." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Config stored and retrieved." << std::endl;
    }

    // ---- Test 11: ping_reply resets missed counter ----
    {
        sooperlooper_engine_monitor mon;
        sooperlooper_engine_monitor::config cfg;
        cfg.stale_threshold = 2;
        mon.set_config(cfg);
        mon.force_state(engine_state::ready);
        mon.ping_missed();
        mon.ping_missed();
        mon.ping_reply("1.7.9", 1);
        mon.ping_missed();
        mon.evaluate();
        if (mon.state() != engine_state::ready)
        {
            std::cerr << "ERROR: Should be ready (missed counter reset by reply)." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] ping_reply resets missed counter." << std::endl;
    }

    // ---- Summary ----
    std::cout << std::endl;
    if (failures == 0)
    {
        std::cout << "All engine monitor tests passed!" << std::endl;
        return 0;
    }
    else
    {
        std::cerr << failures << " test(s) FAILED." << std::endl;
        return 1;
    }
}

/*
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
