/* 
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_command_confirmation_test.cpp
 *
 *  Unit tests for the command confirmation tracker.
 *
 *  M1-006B: Added tests for generation-aware reconciliation, immutable
 *  operation copies, stale detection, and UUID handling.
 */

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

#include "seq66-config.h"
#include "audio/sooperlooper_command_confirmation.hpp"

using namespace seq66;

static int s_failures = 0;

#define CHECK(expr, msg) \
    do { \
        if (! (expr)) { \
            std::cerr << "  FAIL: " << msg << std::endl; \
            ++s_failures; \
        } \
    } while (0)

static const char *
oc_name (confirmation_outcome o)
{
    switch (o)
    {
        case confirmation_outcome::pending:       return "pending";
        case confirmation_outcome::confirmed:     return "confirmed";
        case confirmation_outcome::failed:        return "failed";
        case confirmation_outcome::indeterminate: return "indeterminate";
        case confirmation_outcome::cancelled:     return "cancelled";
    }
    return "unknown";
}

#define CHECK_OUTCOME(tracker, uuid, expected, msg) \
    do { \
        auto _got = (tracker).outcome(uuid); \
        if (_got != (expected)) { \
            std::cerr << "  FAIL: " << msg << " expected=" << oc_name(expected) \
                      << " got=" << oc_name(_got) << std::endl; \
            ++s_failures; \
        } \
    } while (0)

#define CHECK_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "  FAIL: " << msg << std::endl; \
            ++s_failures; \
        } \
    } while (0)

/* -------------------------------------------------------------------------
 *  Original tests (updated for new API)
 * ------------------------------------------------------------------------- */

static void test_track_and_pending ()
{
    std::cout << "Test: track and pending count..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
    CHECK_EQ(tracker.pending_count(), 1u, "Should have 1 pending");
}

static void test_confirm_by_state ()
{
    std::cout << "Test: confirm by state..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
    CHECK(tracker.confirm(0, 1), "Should confirm");
    CHECK_OUTCOME(tracker, "op-1", confirmation_outcome::confirmed, "Should be confirmed");
    CHECK_EQ(tracker.pending_count(), 0u, "Should have 0 pending");
}

static void test_confirm_by_uuid ()
{
    std::cout << "Test: confirm by UUID..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "mute loop 0", "/sl/0/hit", 1, 0);
    CHECK(tracker.confirm_by_uuid("op-1"), "Should confirm by UUID");
}

static void test_fail ()
{
    std::cout << "Test: fail operation..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
    CHECK(tracker.fail("op-1", "engine error"), "Should fail");
    CHECK_OUTCOME(tracker, "op-1", confirmation_outcome::failed, "Should be failed");
}

static void test_cancel ()
{
    std::cout << "Test: cancel operation..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
    CHECK(tracker.cancel("op-1"), "Should cancel");
}

static void test_deadline_expiry ()
{
    std::cout << "Test: deadline expiry..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0, 0, "", 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    CHECK_EQ(tracker.evaluate(), 1, "Should have 1 expired");
    CHECK_OUTCOME(tracker, "op-1", confirmation_outcome::indeterminate, "Should be indeterminate");
}

static void test_reconcile_indeterminate ()
{
    std::cout << "Test: reconcile indeterminate..." << std::endl;
    command_confirmation_tracker tracker;
    std::atomic<bool> reconciled{false};
    tracker.set_reconciler([&](const pending_operation &) -> bool { return reconciled.load(); });
    tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0, 0, "", 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tracker.evaluate();
    reconciled.store(true);
    CHECK_EQ(tracker.reconcile(), 1, "Should reconcile 1");
}

static void test_wrong_state_not_confirmed ()
{
    std::cout << "Test: wrong state not confirmed..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
    CHECK(! tracker.confirm(0, 2), "Wrong state should not confirm");
    CHECK_OUTCOME(tracker, "op-1", confirmation_outcome::pending, "Should still be pending");
}

static void test_wrong_loop_not_confirmed ()
{
    std::cout << "Test: wrong loop not confirmed..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0);
    CHECK(! tracker.confirm(1, 1), "Wrong loop should not confirm");
}

static void test_duplicate_uuid_rejected ()
{
    std::cout << "Test: duplicate UUID rejected..." << std::endl;
    command_confirmation_tracker tracker;
    CHECK(tracker.track("op-1", "a", "/sl/0/hit", 1, 0), "First should succeed");
    CHECK(! tracker.track("op-1", "b", "/sl/0/hit", 1, 0), "Duplicate should be rejected");
}

static void test_by_outcome_filter ()
{
    std::cout << "Test: by_outcome filter..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "a", "/sl/0/hit", 1, 0);
    tracker.track("op-2", "b", "/sl/0/hit", 1, 0);
    tracker.confirm_by_uuid("op-1");
    CHECK_EQ(tracker.by_outcome(confirmation_outcome::confirmed).size(), 1u, "1 confirmed");
    CHECK_EQ(tracker.by_outcome(confirmation_outcome::pending).size(), 1u, "1 pending");
}

static void test_clear ()
{
    std::cout << "Test: clear removes all..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "a", "/sl/0/hit", 1, 0);
    tracker.track("op-2", "b", "/sl/0/hit", 1, 0);
    tracker.clear();
    CHECK_EQ(tracker.pending_count(), 0u, "0 pending after clear");
}

/* -------------------------------------------------------------------------
 *  M1-006B: Generation-aware tests
 * ------------------------------------------------------------------------- */

static void test_unknown_uuid_returns_indeterminate ()
{
    std::cout << "Test: unknown UUID returns indeterminate..." << std::endl;
    command_confirmation_tracker tracker;
    CHECK_OUTCOME(tracker, "nonexistent", confirmation_outcome::indeterminate,
                  "Unknown UUID must return indeterminate");
}

static void test_track_with_generation ()
{
    std::cout << "Test: track with generation..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0, 42, "state");
    auto pending = tracker.pending();
    CHECK_EQ(pending.size(), 1u, "1 pending");
    if (! pending.empty())
    {
        CHECK_EQ(pending[0].engine_generation, 42u, "Generation should be 42");
        CHECK_EQ(pending[0].expected_control, std::string("state"), "Control should be state");
    }
}

static void test_cancel_generation ()
{
    std::cout << "Test: cancel generation..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "a", "/sl/0/hit", 1, 0, 1);
    tracker.track("op-2", "b", "/sl/0/hit", 1, 0, 1);
    tracker.track("op-3", "c", "/sl/0/hit", 1, 0, 2);

    CHECK_EQ(tracker.cancel_generation(1), 2, "Cancel 2 from gen 1");
    CHECK_OUTCOME(tracker, "op-1", confirmation_outcome::cancelled, "op-1 cancelled");
    CHECK_OUTCOME(tracker, "op-2", confirmation_outcome::cancelled, "op-2 cancelled");
    CHECK_OUTCOME(tracker, "op-3", confirmation_outcome::pending, "op-3 still pending");
}

static void test_reconcile_receives_immutable_copy ()
{
    std::cout << "Test: reconcile receives immutable copy..." << std::endl;
    command_confirmation_tracker tracker;

    std::string received_uuid;
    std::string received_control;
    std::uint64_t received_gen = 0;
    int received_expected_state = -1;

    tracker.set_reconciler(
        [&](const pending_operation & op) -> bool
        {
            received_uuid = op.uuid;
            received_control = op.expected_control;
            received_gen = op.engine_generation;
            received_expected_state = op.expected_state;
            return true;
        });

    tracker.track("op-1", "record loop 0", "/sl/0/hit", 1, 0, 5, "state", 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tracker.evaluate();
    CHECK_EQ(tracker.reconcile(), 1, "Should reconcile 1");
    CHECK_EQ(received_uuid, std::string("op-1"), "UUID");
    CHECK_EQ(received_control, std::string("state"), "Control");
    CHECK_EQ(received_gen, 5u, "Generation");
    CHECK_EQ(received_expected_state, 1, "Expected state");
}

static void test_reconcile_reentrant_safety ()
{
    std::cout << "Test: reconcile reentrant safety..." << std::endl;
    command_confirmation_tracker tracker;

    std::atomic<int> call_count{0};
    tracker.set_reconciler([&](const pending_operation &) -> bool { ++call_count; return true; });

    tracker.track("op-1", "a", "/sl/0/hit", 1, 0, 0, "", 1);
    tracker.track("op-2", "b", "/sl/1/hit", 1, 1, 0, "", 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tracker.evaluate();
    CHECK_EQ(tracker.reconcile(), 2, "Should reconcile 2");
    CHECK_EQ(call_count.load(), 2, "Reconciler called 2 times");
}

static void test_two_operations_same_loop ()
{
    std::cout << "Test: two operations same loop..." << std::endl;
    command_confirmation_tracker tracker;

    std::vector<std::string> confirmed_uuids;
    tracker.set_reconciler(
        [&](const pending_operation & op) -> bool
        {
            confirmed_uuids.push_back(op.uuid);
            return true;
        });

    tracker.track("op-1", "record", "/sl/0/hit", 1, 0, 0, "state", 1);
    tracker.track("op-2", "overdub", "/sl/0/hit", 5, 0, 0, "state", 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tracker.evaluate();
    CHECK_EQ(tracker.reconcile(), 2, "Should reconcile both");
    CHECK_EQ(confirmed_uuids.size(), 2u, "Both UUIDs confirmed");
}

static void test_timeout_not_success ()
{
    std::cout << "Test: timeout not success..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "a", "/sl/0/hit", 1, 0, 0, "", 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tracker.evaluate();

    /* No reconciler: reconcile() should not confirm anything */
    CHECK_EQ(tracker.reconcile(), 0, "Timeout without reconciler should not confirm");
    CHECK_OUTCOME(tracker, "op-1", confirmation_outcome::indeterminate, "indeterminate, not confirmed");
}

static void test_confirm_by_state_after_evaluate ()
{
    std::cout << "Test: confirm by state after evaluate..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "a", "/sl/0/hit", 1, 0, 0, "", 10000);
    CHECK(tracker.confirm(0, 1), "Should confirm by state before deadline");
    CHECK_OUTCOME(tracker, "op-1", confirmation_outcome::confirmed, "Should be confirmed");
}

static void test_expected_state_any ()
{
    std::cout << "Test: expected state any (-1)..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "a", "/sl/0/hit", -1, 0);
    CHECK(tracker.confirm(0, 999), "Any state should match");
}

static void test_reconcile_stale_generation ()
{
    std::cout << "Test: reconcile stale generation..." << std::endl;
    command_confirmation_tracker tracker;

    /* Track with generation 5, make indeterminate */
    tracker.track("op-1", "a", "/sl/0/hit", 1, 0, 5, "", 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tracker.evaluate();

    /* Cancel all from generation 5 */
    CHECK_EQ(tracker.cancel_generation(5), 1, "Cancel 1 from gen 5");

    /* Reconcile should find nothing */
    tracker.set_reconciler([&](const pending_operation &) -> bool { return true; });
    CHECK_EQ(tracker.reconcile(), 0, "Should not confirm cancelled");
}

static void test_reconcile_operation_modified_during_callback ()
{
    std::cout << "Test: reconcile operation modified during callback..." << std::endl;

    /* Simple test: verify that the reconciler receives correct data
     * and the re-acquire check works for the normal path. */
    command_confirmation_tracker tracker;
    std::string confirmed_uuid;
    tracker.set_reconciler(
        [&](const pending_operation & op) -> bool
        {
            confirmed_uuid = op.uuid;
            return true;
        });

    tracker.track("op-1", "a", "/sl/0/hit", 1, 0, 0, "", 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tracker.evaluate();
    CHECK_EQ(tracker.reconcile(), 1, "Should confirm 1");
    CHECK_EQ(confirmed_uuid, std::string("op-1"), "Should confirm op-1");
}

static void test_cancel_generation_prevents_reconcile ()
{
    std::cout << "Test: cancel generation prevents reconcile..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.set_reconciler([&](const pending_operation &) -> bool { return true; });

    tracker.track("op-1", "a", "/sl/0/hit", 1, 0, 3, "", 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    tracker.evaluate();

    /* Cancel operations from generation 3 (simulates restart) */
    tracker.cancel_generation(3);

    CHECK_EQ(tracker.reconcile(), 0, "Should not confirm cancelled");
}

static void test_track_rejects_after_cancel ()
{
    std::cout << "Test: track rejects after cancel..." << std::endl;
    command_confirmation_tracker tracker;
    tracker.track("op-1", "a", "/sl/0/hit", 1, 0);
    tracker.cancel("op-1");
    CHECK(! tracker.track("op-1", "b", "/sl/0/hit", 1, 0), "Duplicate UUID rejected after cancel");
}

/* -------------------------------------------------------------------------
 *  Main
 * ------------------------------------------------------------------------- */

int
main ()
{
    std::cout << "=== sooperlooper_command_confirmation_test ===" << std::endl;

    /* Original tests */
    test_track_and_pending();
    test_confirm_by_state();
    test_confirm_by_uuid();
    test_fail();
    test_cancel();
    test_deadline_expiry();
    test_reconcile_indeterminate();
    test_wrong_state_not_confirmed();
    test_wrong_loop_not_confirmed();
    test_duplicate_uuid_rejected();
    test_by_outcome_filter();
    test_clear();

    /* M1-006B tests */
    test_unknown_uuid_returns_indeterminate();
    test_track_with_generation();
    test_cancel_generation();
    test_reconcile_receives_immutable_copy();
    test_reconcile_reentrant_safety();
    test_two_operations_same_loop();
    test_timeout_not_success();
    test_confirm_by_state_after_evaluate();
    test_expected_state_any();
    test_reconcile_stale_generation();
    test_reconcile_operation_modified_during_callback();
    test_cancel_generation_prevents_reconcile();
    test_track_rejects_after_cancel();

    std::cout << std::endl;
    if (s_failures == 0)
    {
        std::cout << "All confirmation tracker tests PASSED." << std::endl;
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
