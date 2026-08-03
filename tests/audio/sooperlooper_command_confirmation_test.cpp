/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_command_confirmation_test.cpp
 *
 *  Unit tests for the command confirmation tracker.
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "seq66-config.h"
#include "audio/sooperlooper_command_confirmation.hpp"

using namespace seq66;

int
main ()
{
    std::cout << "Testing command_confirmation_tracker..." << std::endl;
    int failures = 0;

    // ---- Test 1: track and pending count ----
    {
        command_confirmation_tracker tracker;
        tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
        if (tracker.pending_count() != 1)
        {
            std::cerr << "ERROR: Should have 1 pending." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Track and pending count." << std::endl;
    }

    // ---- Test 2: confirm by state ----
    {
        command_confirmation_tracker tracker;
        tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
        bool confirmed = tracker.confirm(0, 1);
        if (!confirmed)
        {
            std::cerr << "ERROR: Should confirm." << std::endl;
            ++failures;
        }
        if (tracker.outcome("op-1") != confirmation_outcome::confirmed)
        {
            std::cerr << "ERROR: Should be confirmed." << std::endl;
            ++failures;
        }
        if (tracker.pending_count() != 0)
        {
            std::cerr << "ERROR: Should have 0 pending." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Confirm by state." << std::endl;
    }

    // ---- Test 3: confirm by UUID ----
    {
        command_confirmation_tracker tracker;
        tracker.track("op-1", "mute loop 0", "/sl/0/hit", 1, 0);
        bool confirmed = tracker.confirm_by_uuid("op-1");
        if (!confirmed)
        {
            std::cerr << "ERROR: Should confirm by UUID." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Confirm by UUID." << std::endl;
    }

    // ---- Test 4: fail operation ----
    {
        command_confirmation_tracker tracker;
        tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
        bool failed = tracker.fail("op-1", "engine error");
        if (!failed)
        {
            std::cerr << "ERROR: Should fail." << std::endl;
            ++failures;
        }
        if (tracker.outcome("op-1") != confirmation_outcome::failed)
        {
            std::cerr << "ERROR: Should be failed." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Fail operation." << std::endl;
    }

    // ---- Test 5: cancel operation ----
    {
        command_confirmation_tracker tracker;
        tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
        bool cancelled = tracker.cancel("op-1");
        if (!cancelled)
        {
            std::cerr << "ERROR: Should cancel." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Cancel operation." << std::endl;
    }

    // ---- Test 6: deadline expiry ----
    {
        command_confirmation_tracker tracker;
        tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0, 1);  // 1ms deadline
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        int expired = tracker.evaluate();
        if (expired != 1)
        {
            std::cerr << "ERROR: Should have 1 expired." << std::endl;
            ++failures;
        }
        if (tracker.outcome("op-1") != confirmation_outcome::indeterminate)
        {
            std::cerr << "ERROR: Should be indeterminate." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Deadline expiry." << std::endl;
    }

    // ---- Test 7: reconcile indeterminate ----
    {
        command_confirmation_tracker tracker;
        bool reconciled = false;
        tracker.set_reconciler([&](int) { return reconciled; });
        tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        tracker.evaluate();
        reconciled = true;
        int count = tracker.reconcile();
        if (count != 1)
        {
            std::cerr << "ERROR: Should reconcile 1." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Reconcile indeterminate." << std::endl;
    }

    // ---- Test 8: wrong state not confirmed ----
    {
        command_confirmation_tracker tracker;
        tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
        bool confirmed = tracker.confirm(0, 2);  // wrong state
        if (confirmed)
        {
            std::cerr << "ERROR: Wrong state should not confirm." << std::endl;
            ++failures;
        }
        if (tracker.outcome("op-1") != confirmation_outcome::pending)
        {
            std::cerr << "ERROR: Should still be pending." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Wrong state not confirmed." << std::endl;
    }

    // ---- Test 9: wrong loop not confirmed ----
    {
        command_confirmation_tracker tracker;
        tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
        bool confirmed = tracker.confirm(1, 1);  // wrong loop
        if (confirmed)
        {
            std::cerr << "ERROR: Wrong loop should not confirm." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Wrong loop not confirmed." << std::endl;
    }

    // ---- Test 10: duplicate UUID rejected ----
    {
        command_confirmation_tracker tracker;
        bool ok1 = tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
        bool ok2 = tracker.track("op-1", "record loop 0 again", "/sl/0/hit", 1, 0);
        if (!ok1 || ok2)
        {
            std::cerr << "ERROR: Duplicate UUID should be rejected." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Duplicate UUID rejected." << std::endl;
    }

    // ---- Test 11: by_outcome filter ----
    {
        command_confirmation_tracker tracker;
        tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
        tracker.track("op-2", "mute loop 0", "/sl/0/hit", 1, 0);
        tracker.confirm_by_uuid("op-1");
        auto confirmed = tracker.by_outcome(confirmation_outcome::confirmed);
        auto pending = tracker.by_outcome(confirmation_outcome::pending);
        if (confirmed.size() != 1 || pending.size() != 1)
        {
            std::cerr << "ERROR: by_outcome filter wrong." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] by_outcome filter." << std::endl;
    }

    // ---- Test 12: clear removes all ----
    {
        command_confirmation_tracker tracker;
        tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
        tracker.track("op-2", "mute loop 0", "/sl/0/hit", 1, 0);
        tracker.clear();
        if (tracker.pending_count() != 0)
        {
            std::cerr << "ERROR: Should have 0 pending after clear." << std::endl;
            ++failures;
        }
        std::cout << "  [PASS] Clear removes all." << std::endl;
    }

    // ---- Summary ----
    std::cout << std::endl;
    if (failures == 0)
    {
        std::cout << "All confirmation tracker tests passed!" << std::endl;
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
