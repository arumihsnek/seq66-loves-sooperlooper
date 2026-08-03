/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_fault_injection_test.cpp
 *
 *  M1-007: Negative and fault-injection matrix.
 *
 *  Tests that the SooperLooper integration components handle adverse
 *  conditions without crash, deadlock, or false confirmed state.
 *
 *  Coverage:
 *  - Delayed callbacks (messages arriving after deadline)
 *  - Duplicated callbacks (same message sent twice)
 *  - Reordered callbacks (messages arriving out of order)
 *  - Malformed values (NaN, Inf, negative indexes)
 *  - Queue overflow (too many messages)
 *  - Generation mismatch (stale events)
 *  - Receiver shutdown during active callbacks
 *  - Concurrent access to observer cache
 *  - Command confirmation timeout
 *  - Empty/null argument handling
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <cmath>
#include <functional>

#include "seq66-config.h"
#include "audio/sooperlooper_receiver.hpp"
#include "audio/sooperlooper_observed_state.hpp"
#include "audio/sooperlooper_engine_monitor.hpp"
#include "audio/sooperlooper_command_confirmation.hpp"
#include "audio/sooperlooper_protocol.hpp"

using namespace seq66;

static int failures = 0;

static void check (bool condition, const std::string & name)
{
    if (! condition)
    {
        std::cerr << "  FAIL: " << name << std::endl;
        ++failures;
    }
    else
    {
        std::cout << "  [PASS] " << name << std::endl;
    }
}

/**
 *  Test 1: Queue overflow — messages beyond capacity are dropped,
 *  never crash, and dropped_count is accurate.
 */
static void test_queue_overflow ()
{
    std::cout << "\n--- Queue overflow ---" << std::endl;

    sooperlooper_receiver receiver(0, 4);  /* Tiny queue: 4 entries */
    receiver.start();
    int port = receiver.port();

    /* Send 20 messages to overflow the queue. */
    lo_address addr = lo_address_new_from_url(
        ("osc.udp://127.0.0.1:" + std::to_string(port)).c_str());

    for (int i = 0; i < 20; ++i)
    {
        lo_send(addr, "/test/overflow", "i", i);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    /* Verify drops were counted. */
    unsigned long long dropped = receiver.dropped_count();
    check(dropped > 0, "dropped_count > 0 after overflow");
    check(dropped <= 20, "dropped_count <= 20");

    /* Drain remaining messages. */
    sooperlooper_receiver::receiver_event event;
    int received = 0;
    while (receiver.poll_event(event))
        ++received;
    check(received >= 1, "at least 1 message received despite overflow");

    lo_address_free(addr);
    receiver.stop();
}

/**
 *  Test 2: Duplicated callbacks — same message sent twice is handled
 *  gracefully (no crash, no corruption).
 */
static void test_duplicated_callbacks ()
{
    std::cout << "\n--- Duplicated callbacks ---" << std::endl;

    sooperlooper_receiver receiver(0);
    receiver.start();
    int port = receiver.port();

    lo_address addr = lo_address_new_from_url(
        ("osc.udp://127.0.0.1:" + std::to_string(port)).c_str());

    /* Send the same message 5 times. */
    for (int i = 0; i < 5; ++i)
    {
        lo_send(addr, "/sl/0/set", "sf", "state", 1.0f);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    /* All 5 should be received (no deduplication at receiver level). */
    sooperlooper_receiver::receiver_event event;
    int count = 0;
    while (receiver.poll_event(event))
        ++count;
    check(count == 5, "all 5 duplicate messages received");

    lo_address_free(addr);
    receiver.stop();
}

/**
 *  Test 3: Reordered callbacks — messages sent in reverse order are
 *  received and applied correctly.
 */
static void test_reordered_callbacks ()
{
    std::cout << "\n--- Reordered callbacks ---" << std::endl;

    sooperlooper_receiver receiver(0);
    receiver.start();
    int port = receiver.port();

    lo_address addr = lo_address_new_from_url(
        ("osc.udp://127.0.0.1:" + std::to_string(port)).c_str());

    /* Send messages in reverse order. */
    for (int i = 4; i >= 0; --i)
    {
        lo_send(addr, "/test/reorder", "i", i);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    /* All 5 should be received. */
    sooperlooper_receiver::receiver_event event;
    int count = 0;
    while (receiver.poll_event(event))
        ++count;
    check(count == 5, "all 5 reordered messages received");

    lo_address_free(addr);
    receiver.stop();
}

/**
 *  Test 4: Malformed values — NaN and Inf are rejected by the
 *  observed state cache.
 */
static void test_malformed_values ()
{
    std::cout << "\n--- Malformed values ---" << std::endl;

    sooperlooper_observed_cache cache;

    /* NaN should be rejected. */
    sooperlooper_receiver::receiver_event nan_event;
    nan_event.path = "/sl/0/callback";
    nan_event.types = "isf";
    nan_event.args = {"in_peak_meter", "nan"};
    nan_event.timestamp_us = 1000;
    nan_event.generation = 0;

    bool applied = cache.apply_event(nan_event);
    check(! applied, "NaN value rejected");

    /* Inf should be rejected. */
    sooperlooper_receiver::receiver_event inf_event;
    inf_event.path = "/sl/0/callback";
    inf_event.types = "isf";
    inf_event.args = {"in_peak_meter", "inf"};
    inf_event.timestamp_us = 1001;
    inf_event.generation = 0;

    applied = cache.apply_event(inf_event);
    check(! applied, "Inf value rejected");

    /* Negative Inf should be rejected. */
    sooperlooper_receiver::receiver_event neg_inf_event;
    neg_inf_event.path = "/sl/0/callback";
    neg_inf_event.types = "isf";
    neg_inf_event.args = {"in_peak_meter", "-inf"};
    neg_inf_event.timestamp_us = 1002;
    neg_inf_event.generation = 0;

    applied = cache.apply_event(neg_inf_event);
    check(! applied, "Negative Inf value rejected");

    /* Valid value should still work. */
    sooperlooper_receiver::receiver_event valid_event;
    valid_event.path = "/sl/0/callback";
    valid_event.types = "isf";
    valid_event.args = {"in_peak_meter", "1.5"};
    valid_event.timestamp_us = 1003;
    valid_event.generation = 0;

    applied = cache.apply_event(valid_event);
    check(applied, "Valid value accepted after malformed values");
}

/**
 *  Test 5: Negative loop index — rejected by observed state cache.
 */
static void test_negative_loop_index ()
{
    std::cout << "\n--- Negative loop index ---" << std::endl;

    sooperlooper_observed_cache cache;

    sooperlooper_receiver::receiver_event event;
    event.path = "/sl/-1/callback";
    event.types = "isf";
    event.args = {"state", "1"};
    event.timestamp_us = 1000;
    event.generation = 0;

    bool applied = cache.apply_event(event);
    check(! applied, "Negative loop index rejected");
}

/**
 *  Test 6: Generation mismatch — stale events are rejected.
 */
static void test_generation_mismatch ()
{
    std::cout << "\n--- Generation mismatch ---" << std::endl;

    sooperlooper_observed_cache cache;

    /* Set generation to 1. */
    cache.set_generation(1);

    /* Event with generation 0 should be rejected. */
    sooperlooper_receiver::receiver_event stale_event;
    stale_event.path = "/sl/0/callback";
    stale_event.types = "isf";
    stale_event.args = {"state", "1"};
    stale_event.timestamp_us = 1000;
    stale_event.generation = 0;

    bool applied = cache.apply_event(stale_event);
    check(! applied, "Stale generation event rejected");

    /* Event with matching generation should be accepted. */
    sooperlooper_receiver::receiver_event valid_event;
    valid_event.path = "/sl/0/callback";
    valid_event.types = "isf";
    valid_event.args = {"state", "1"};
    valid_event.timestamp_us = 1001;
    valid_event.generation = 1;

    applied = cache.apply_event(valid_event);
    check(applied, "Matching generation event accepted");
}

/**
 *  Test 7: Receiver shutdown during active message flow — no crash.
 */
static void test_shutdown_during_flow ()
{
    std::cout << "\n--- Shutdown during message flow ---" << std::endl;

    sooperlooper_receiver receiver(0);
    receiver.start();
    int port = receiver.port();

    lo_address addr = lo_address_new_from_url(
        ("osc.udp://127.0.0.1:" + std::to_string(port)).c_str());

    /* Send messages in a tight loop while shutting down. */
    std::atomic<bool> sending{true};
    std::thread sender([&]()
    {
        int i = 0;
        while (sending.load())
        {
            lo_send(addr, "/test/flow", "i", i++);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    /* Let some messages through, then shut down. */
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    receiver.stop();
    sending.store(false);
    sender.join();

    /* No crash = pass. */
    check(!receiver.started(), "receiver stopped during flow");

    lo_address_free(addr);
}

/**
 *  Test 8: Concurrent cache access — multiple threads reading and
 *  writing does not crash or deadlock.
 */
static void test_concurrent_cache_access ()
{
    std::cout << "\n--- Concurrent cache access ---" << std::endl;

    sooperlooper_observed_cache cache;
    std::atomic<bool> done{false};
    std::atomic<int> write_count{0};
    std::atomic<int> read_count{0};

    /* Writer thread. */
    std::thread writer([&]()
    {
        int gen = 0;
        while (! done.load())
        {
            cache.set_generation(++gen);
            sooperlooper_receiver::receiver_event event;
            event.path = "/sl/0/callback";
            event.types = "isf";
            event.args = {"0", "state", "1"};
            event.timestamp_us = gen;
            event.generation = gen;
            cache.apply_event(event);
            write_count.fetch_add(1);
        }
    });

    /* Reader threads. */
    std::vector<std::thread> readers;
    for (int r = 0; r < 3; ++r)
    {
        readers.emplace_back([&]()
        {
            while (! done.load())
            {
                auto snap = cache.snapshot();
                (void) snap;
                read_count.fetch_add(1);
            }
        });
    }

    /* Run for 100ms. */
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    done.store(true);

    writer.join();
    for (auto & t : readers)
        t.join();

    check(write_count.load() > 0, "writer made progress");
    check(read_count.load() > 0, "readers made progress");
    check(cache.generation() > 0, "generation advanced during concurrent access");
}

/**
 *  Test 9: Command confirmation timeout — expired operations become
 *  indeterminate, not confirmed.
 */
static void test_confirmation_timeout ()
{
    std::cout << "\n--- Command confirmation timeout ---" << std::endl;

    command_confirmation_tracker tracker;
    /* Track an operation. */
    bool tracked = tracker.track(
        "op-timeout", "record loop 0", "/sl/0/hit", 1, 0, 10);  /* 10ms deadline */
    check(tracked, "operation tracked");

    /* Wait for deadline to expire. */
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    int expired = tracker.evaluate();
    check(expired == 1, "one operation expired");

    /* The operation should be indeterminate, NOT confirmed. */
    auto outcome = tracker.outcome("op-timeout");
    check(outcome == confirmation_outcome::indeterminate,
          "timeout produces indeterminate, not confirmed");
}

/**
 *  Test 10: Empty argument handling — events with missing arguments
 *  are rejected safely.
 */
static void test_empty_arguments ()
{
    std::cout << "\n--- Empty argument handling ---" << std::endl;

    sooperlooper_observed_cache cache;

    /* Event with empty args. */
    sooperlooper_receiver::receiver_event event;
    event.path = "/sl/0/callback";
    event.types = "isf";
    event.args = {};  /* Empty args. */
    event.timestamp_us = 1000;
    event.generation = 0;

    bool applied = cache.apply_event(event);
    check(! applied, "empty args rejected");

    /* Event with too few args. */
    sooperlooper_receiver::receiver_event short_event;
    short_event.path = "/sl/0/callback";
    short_event.types = "isf";
    short_event.args = {"0"};  /* Missing control and value. */
    short_event.timestamp_us = 1001;
    short_event.generation = 0;

    applied = cache.apply_event(short_event);
    check(! applied, "too few args rejected");
}

/**
 *  Test 11: Engine monitor stale threshold — repeated missed pings
 *  trigger deterministic state transitions.
 */
static void test_monitor_stale_threshold ()
{
    std::cout << "\n--- Monitor stale threshold ---" << std::endl;

    sooperlooper_engine_monitor mon;
    sooperlooper_engine_monitor::config cfg;
    cfg.stale_threshold = 2;
    mon.set_config(cfg);

    mon.ping_sent();
    mon.ping_reply("1.7.9", 4);
    check(mon.state() == engine_state::reconciling,
          "reconciling after ping reply");

    mon.force_state(engine_state::ready);

    /* Missed pings below threshold. */
    mon.ping_missed();
    mon.evaluate();
    check(mon.state() == engine_state::ready,
          "still ready after 1 missed ping (threshold=2)");

    mon.ping_missed();
    mon.evaluate();
    check(mon.state() == engine_state::stale,
          "stale after 2 missed pings (threshold=2)");

    /* Continue missing -> engine_offline. */
    mon.ping_missed();
    mon.ping_missed();
    mon.evaluate();
    check(mon.state() == engine_state::engine_offline,
          "engine_offline after 4 missed pings");
}

/**
 *  Test 12: Receiver handler registration — registering the same
 *  path twice replaces the old handler (no crash).
 */
static void test_duplicate_handler_registration ()
{
    std::cout << "\n--- Duplicate handler rejected ---" << std::endl;

    sooperlooper_receiver receiver(0);
    receiver.start();
    int port = receiver.port();

    int call_count_a = 0;
    int call_count_b = 0;

    bool first = receiver.handle("/sl/0/dup", "f",
        [&](const std::vector<std::string> &, long long)
        { ++call_count_a; });

    /* Register again on same path — should be rejected. */
    bool second = receiver.handle("/sl/0/dup", "f",
        [&](const std::vector<std::string> &, long long)
        { ++call_count_b; });

    lo_address addr = lo_address_new_from_url(
        ("osc.udp://127.0.0.1:" + std::to_string(port)).c_str());
    lo_send(addr, "/sl/0/dup", "f", 1.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    /* Dispatch to trigger handlers. */
    receiver.dispatch();

    check(first, "first handler registration succeeds");
    check(! second, "duplicate handler registration rejected");
    check(call_count_a == 1, "first handler still active");
    check(call_count_b == 0, "duplicate handler not called");

    lo_address_free(addr);
    receiver.stop();
}

/**
 *  Test 13: Command confirmation — fail and cancel operations
 *  produce correct outcomes.
 */
static void test_confirmation_fail_cancel ()
{
    std::cout << "\n--- Confirmation fail and cancel ---" << std::endl;

    command_confirmation_tracker tracker;

    tracker.track("op-fail", "overdub loop 1", "/sl/1/hit", 1, 1);
    tracker.track("op-cancel", "mute loop 2", "/sl/2/hit", 1, 2);

    bool failed = tracker.fail("op-fail", "engine error");
    check(failed, "fail returns true");
    check(tracker.outcome("op-fail") == confirmation_outcome::failed,
          "outcome is failed");

    bool cancelled = tracker.cancel("op-cancel");
    check(cancelled, "cancel returns true");
    check(tracker.outcome("op-cancel") == confirmation_outcome::cancelled,
          "outcome is cancelled");

    check(tracker.pending_count() == 0, "no pending after fail+cancel");
}

/**
 *  Test 14: Reorder resilience — applying events in different order
 *  still produces correct final state.
 */
static void test_reorder_resilience ()
{
    std::cout << "\n--- Reorder resilience ---" << std::endl;

    sooperlooper_observed_cache cache;

    /* Apply state=2, then state=1, then state=3. */
    auto make_event = [&](int state_val, long long ts)
    {
        sooperlooper_receiver::receiver_event e;
        e.path = "/sl/0/callback";
        e.types = "isf";
        e.args = {"state", std::to_string(state_val)};
        e.timestamp_us = ts;
        e.generation = 0;
        return e;
    };

    cache.apply_event(make_event(2, 3000));
    cache.apply_event(make_event(1, 1000));  /* Earlier timestamp. */
    cache.apply_event(make_event(3, 5000));  /* Latest timestamp. */

    auto snap = cache.snapshot();
    /* The latest timestamp should win. */
    check(snap.loops.count(0) && snap.loops[0].state.present,
          "reorder resilience — state present after out-of-order apply");
    check(snap.loops[0].state.value == 3,
          "reorder resilience — latest timestamp value wins");
}

int
main ()
{
    std::cout << "M1-007: Negative and fault-injection matrix" << std::endl;
    std::cout << "===========================================" << std::endl;

    test_queue_overflow();
    test_duplicated_callbacks();
    test_reordered_callbacks();
    test_malformed_values();
    test_negative_loop_index();
    test_generation_mismatch();
    test_shutdown_during_flow();
    test_concurrent_cache_access();
    test_confirmation_timeout();
    test_empty_arguments();
    test_monitor_stale_threshold();
    test_duplicate_handler_registration();
    test_confirmation_fail_cancel();
    test_reorder_resilience();

    std::cout << "\n===========================================" << std::endl;
    if (failures == 0)
    {
        std::cout << "All fault-injection tests passed." << std::endl;
    }
    else
    {
        std::cerr << failures << " test(s) FAILED." << std::endl;
    }

    return failures;
}
