/* 
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_subscription_test.cpp
 *
 *  M1-005B: Comprehensive test for subscription wire protocol.
 *
 *  Verifies:
 *    - Exact OSC path for each subscription type
 *    - Exact type signature (ss for register_update, siss for register_auto_update)
 *    - Exact arguments (control, return_url, return_path, interval)
 *    - Interval sent as integer, not float
 *    - Loop and global subscriptions
 *    - Unregister methods
 *    - Callback URL validity
 *    - Callback path validity
 *    - Interval range validation (10..100)
 *    - Shutdown/cancel_subscriptions
 *    - Generation tracking
 *    - No receiver bound error
 *
 *  Build command (mirrors audio-core.yml):
 *      g++ \
 *          -std=c++17 \
 *          -Wall -Wextra -Wpedantic -Werror \
 *          -pthread \
 *          -I.ci/include \
 *          -Ilibseq66/include \
 *          libseq66/src/audio/audio_clip.cpp \
 *          libseq66/src/audio/sooperlooper_client.cpp \
 *          libseq66/src/audio/sooperlooper_protocol.cpp \
 *          libseq66/src/audio/sooperlooper_receiver.cpp \
 *          tests/audio/sooperlooper_subscription_test.cpp \
 *          -llo \
 *          -o .ci/bin/sooperlooper_subscription_test
 *
 *  Run: .ci/bin/sooperlooper_subscription_test
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>

#include "seq66-config.h"

#include "audio/sooperlooper_client.hpp"
#include "audio/sooperlooper_protocol.hpp"
#include "audio/sooperlooper_receiver.hpp"

using namespace seq66;

/* -------------------------------------------------------------------------
 *  Captured message structure for fake engine verification
 * ------------------------------------------------------------------------- */

struct captured_message
{
    std::string path;
    std::string types;
    std::vector<std::string> args;
    long long timestamp_us;
};

/* -------------------------------------------------------------------------
 *  Fake OSC server that captures sent messages
 *
 *  We use a real sooperlooper_receiver to capture messages sent by the
 *  client.  The client sends to the receiver's port, and the receiver's
 *  handler captures the exact path, types, and arguments.
 * ------------------------------------------------------------------------- */

class fake_engine
{
private:
    sooperlooper_receiver m_receiver;
    std::vector<captured_message> m_messages;
    bool m_started{false};

public:
    fake_engine () : m_receiver(0, 4096) {}

    bool start ()
    {
        if (m_started)
            return true;
        if (! m_receiver.start())
            return false;

        /* Register a catch-all handler that captures everything. */
        m_receiver.handle("", "",
            [this](const std::vector<std::string> & args, long long ts)
            {
                /* We need the path and types too, but the handler
                 * callback doesn't provide them directly.  We'll use
                 * poll_event instead for exact verification. */
                (void) args;
                (void) ts;
            });

        m_started = true;
        return true;
    }

    void stop ()
    {
        m_receiver.stop();
        m_started = false;
    }

    int port () const { return m_receiver.port(); }
    bool started () const { return m_started; }

    sooperlooper_receiver & receiver () { return m_receiver; }

    /**
     *  Poll for a captured message.
     *  Returns true if a message was available, with its path, types,
     *  and args populated.
     */
    bool poll_message (captured_message & msg)
    {
        sooperlooper_receiver::receiver_event event;
        if (! m_receiver.poll_event(event))
            return false;
        msg.path = event.path;
        msg.types = event.types;
        msg.args = event.args;
        msg.timestamp_us = event.timestamp_us;
        return true;
    }

    /**
     *  Wait for a message with a specific path prefix, with timeout.
     */
    bool wait_message_prefix
    (
        const std::string & prefix,
        captured_message & msg,
        int timeout_ms = 2000
    )
    {
        auto start = std::chrono::steady_clock::now();
        while (true)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= timeout_ms)
                return false;

            sooperlooper_receiver::receiver_event event;
            if (m_receiver.poll_event(event))
            {
                if (prefix.empty() ||
                    event.path.compare(0, prefix.size(), prefix) == 0)
                {
                    msg.path = event.path;
                    msg.types = event.types;
                    msg.args = event.args;
                    msg.timestamp_us = event.timestamp_us;
                    return true;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
};

/* -------------------------------------------------------------------------
 *  Test helpers
 * ------------------------------------------------------------------------- */

static int s_failures = 0;

#define CHECK(expr, msg) \
    do { \
        if (! (expr)) { \
            std::cerr << "  FAIL: " << msg << "  (" #expr ")" << std::endl; \
            ++s_failures; \
        } \
    } while (0)

#define CHECK_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "  FAIL: " << msg << "  expected='" << (b) \
                      << "' got='" << (a) << "'" << std::endl; \
            ++s_failures; \
        } \
    } while (0)

/* -------------------------------------------------------------------------
 *  Test 1: subscribe_loop sends correct wire format
 *
 *  Path: /sl/<index>/register_update
 *  Types: "sss" (control, return_url, return_path)
 *  Args: [control_name, "osc.udp://127.0.0.1:<port>", "/reply"]
 * ------------------------------------------------------------------------- */

static void test_subscribe_loop_wire_format ()
{
    std::cout << "Test: subscribe_loop wire format..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    bool sent = client.subscribe_loop(0, loop_control::state);
    CHECK(sent, "subscribe_loop should succeed");

    captured_message msg;
    bool got = engine.wait_message_prefix("/sl/0/register", msg);
    CHECK(got, "Should receive register_update message");
    if (got)
    {
        CHECK_EQ(msg.path, "/sl/0/register_update",
                 "Exact path must be /sl/0/register_update");
        CHECK_EQ(msg.types, "sss",
                 "Type signature must be sss (control, return_url, return_path)");
        CHECK(msg.args.size() >= 3, "Must have at least 3 arguments");
        if (msg.args.size() >= 3)
        {
            CHECK_EQ(msg.args[0], "state",
                     "First arg must be control name 'state'");
            CHECK(msg.args[1].find("osc.udp://127.0.0.1:") == 0,
                  "return_url must be valid osc.udp URL");
            CHECK_EQ(msg.args[2], "/reply",
                     "return_path must be '/reply'");
        }
    }

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 2: subscribe_loop_auto sends correct wire format
 *
 *  Path: /sl/<index>/register_auto_update
 *  Types: "siss" (control, interval_ms as INT, return_url, return_path)
 *  Args: [control_name, interval_as_int, "osc.udp://...", "/reply"]
 * ------------------------------------------------------------------------- */

static void test_subscribe_loop_auto_wire_format ()
{
    std::cout << "Test: subscribe_loop_auto wire format..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    bool sent = client.subscribe_loop_auto(1, loop_control::loop_pos, 50);
    CHECK(sent, "subscribe_loop_auto should succeed");

    captured_message msg;
    bool got = engine.wait_message_prefix("/sl/1/register_auto", msg);
    CHECK(got, "Should receive register_auto_update message");
    if (got)
    {
        CHECK_EQ(msg.path, "/sl/1/register_auto_update",
                 "Exact path must be /sl/1/register_auto_update");
        CHECK_EQ(msg.types, "siss",
                 "Type signature must be siss (control, int interval, return_url, return_path)");
        CHECK(msg.args.size() >= 4, "Must have at least 4 arguments");
        if (msg.args.size() >= 4)
        {
            CHECK_EQ(msg.args[0], "loop_pos",
                     "First arg must be control name 'loop_pos'");
            /* Verify interval is sent as integer (i type tag), not float.
             * The receiver converts 'i' to string, so we check the raw
             * value matches the integer we sent. */
            CHECK_EQ(msg.args[1], "50",
                     "Interval must be 50 (integer, not float)");
            CHECK(msg.args[2].find("osc.udp://127.0.0.1:") == 0,
                  "return_url must be valid osc.udp URL");
            CHECK_EQ(msg.args[3], "/reply",
                     "return_path must be '/reply'");
        }
    }

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 3: subscribe_global sends correct wire format
 *
 *  Path: /register_update
 *  Types: "sss" (control, return_url, return_path)
 *  Args: [control_name, "osc.udp://...", "/reply"]
 * ------------------------------------------------------------------------- */

static void test_subscribe_global_wire_format ()
{
    std::cout << "Test: subscribe_global wire format..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    bool sent = client.subscribe_global(global_control::tempo);
    CHECK(sent, "subscribe_global should succeed");

    captured_message msg;
    bool got = engine.wait_message_prefix("/register", msg);
    CHECK(got, "Should receive register_update message");
    if (got)
    {
        CHECK_EQ(msg.path, "/register_update",
                 "Exact path must be /register_update");
        CHECK_EQ(msg.types, "sss",
                 "Type signature must be sss");
        CHECK(msg.args.size() >= 3, "Must have at least 3 arguments");
        if (msg.args.size() >= 3)
        {
            CHECK_EQ(msg.args[0], "tempo",
                     "First arg must be control name 'tempo'");
            CHECK(msg.args[1].find("osc.udp://127.0.0.1:") == 0,
                  "return_url must be valid osc.udp URL");
            CHECK_EQ(msg.args[2], "/reply",
                     "return_path must be '/reply'");
        }
    }

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 4: subscribe_global_auto sends correct wire format
 *
 *  Path: /register_auto_update
 *  Types: "siss" (control, interval_ms as INT, return_url, return_path)
 *  Args: [control_name, interval_as_int, "osc.udp://...", "/reply"]
 * ------------------------------------------------------------------------- */

static void test_subscribe_global_auto_wire_format ()
{
    std::cout << "Test: subscribe_global_auto wire format..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    bool sent = client.subscribe_global_auto(global_control::tempo, 30);
    CHECK(sent, "subscribe_global_auto should succeed");

    captured_message msg;
    bool got = engine.wait_message_prefix("/register_auto", msg);
    CHECK(got, "Should receive register_auto_update message");
    if (got)
    {
        CHECK_EQ(msg.path, "/register_auto_update",
                 "Exact path must be /register_auto_update");
        CHECK_EQ(msg.types, "siss",
                 "Type signature must be siss");
        CHECK(msg.args.size() >= 4, "Must have at least 4 arguments");
        if (msg.args.size() >= 4)
        {
            CHECK_EQ(msg.args[0], "tempo",
                     "First arg must be control name 'tempo'");
            CHECK_EQ(msg.args[1], "30",
                     "Interval must be 30 (integer, not float)");
            CHECK(msg.args[2].find("osc.udp://127.0.0.1:") == 0,
                  "return_url must be valid osc.udp URL");
            CHECK_EQ(msg.args[3], "/reply",
                     "return_path must be '/reply'");
        }
    }

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 5: interval range validation
 * ------------------------------------------------------------------------- */

static void test_interval_range_validation ()
{
    std::cout << "Test: interval range validation..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    /* Below minimum */
    bool sent = client.subscribe_loop_auto(0, loop_control::state, 5);
    CHECK(! sent, "Interval 5ms should be rejected (< 10)");

    /* Above maximum */
    sent = client.subscribe_loop_auto(0, loop_control::state, 200);
    CHECK(! sent, "Interval 200ms should be rejected (> 100)");

    /* At boundaries */
    sent = client.subscribe_loop_auto(0, loop_control::state, 10);
    CHECK(sent, "Interval 10ms should be accepted (minimum)");

    sent = client.subscribe_loop_auto(0, loop_control::state, 100);
    CHECK(sent, "Interval 100ms should be accepted (maximum)");

    /* Global auto also validates */
    sent = client.subscribe_global_auto(global_control::tempo, 5);
    CHECK(! sent, "Global auto interval 5ms should be rejected");

    sent = client.subscribe_global_auto(global_control::tempo, 10);
    CHECK(sent, "Global auto interval 10ms should be accepted");

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 6: loop index validation
 * ------------------------------------------------------------------------- */

static void test_loop_index_validation ()
{
    std::cout << "Test: loop index validation..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    /* Valid indexes: 0, 1, 2, ..., -1 (all), -3 (selected) */
    bool sent = client.subscribe_loop(0, loop_control::state);
    CHECK(sent, "Loop index 0 should be accepted");

    sent = client.subscribe_loop(-1, loop_control::state);
    CHECK(sent, "Loop index -1 (all) should be accepted");

    sent = client.subscribe_loop(-3, loop_control::state);
    CHECK(sent, "Loop index -3 (selected) should be accepted");

    /* Invalid: -2 is reserved, -4 is out of range */
    sent = client.subscribe_loop(-2, loop_control::state);
    CHECK(! sent, "Loop index -2 should be rejected (reserved)");

    sent = client.subscribe_loop(-4, loop_control::state);
    CHECK(! sent, "Loop index -4 should be rejected");

    sent = client.subscribe_loop(100, loop_control::state);
    CHECK(sent, "Loop index 100 should be accepted (valid positive)");

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 7: unregister methods send correct wire format
 * ------------------------------------------------------------------------- */

static void test_unregister_wire_format ()
{
    std::cout << "Test: unregister wire format..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    /* unregister_loop */
    bool sent = client.unsubscribe_loop(0, loop_control::state);
    CHECK(sent, "unsubscribe_loop should succeed");

    captured_message msg;
    bool got = engine.wait_message_prefix("/sl/0/unregister", msg);
    CHECK(got, "Should receive unregister_update message");
    if (got)
    {
        CHECK_EQ(msg.path, "/sl/0/unregister_update",
                 "Path must be /sl/0/unregister_update");
        CHECK_EQ(msg.types, "sss",
                 "Type signature must be sss");
        CHECK(msg.args.size() >= 3, "Must have 3 args");
        if (msg.args.size() >= 3)
        {
            CHECK_EQ(msg.args[0], "state", "Control must be 'state'");
            CHECK(msg.args[1].find("osc.udp://127.0.0.1:") == 0,
                  "return_url must be valid");
            CHECK_EQ(msg.args[2], "/reply", "return_path must be '/reply'");
        }
    }

    /* unregister_loop_auto */
    sent = client.unsubscribe_loop_auto(1, loop_control::loop_pos);
    CHECK(sent, "unsubscribe_loop_auto should succeed");

    got = engine.wait_message_prefix("/sl/1/unregister_auto", msg);
    CHECK(got, "Should receive unregister_auto_update message");
    if (got)
    {
        CHECK_EQ(msg.path, "/sl/1/unregister_auto_update",
                 "Path must be /sl/1/unregister_auto_update");
        CHECK_EQ(msg.types, "sss",
                 "Type signature must be sss (no interval in unregister)");
    }

    /* unregister_global */
    sent = client.unsubscribe_global(global_control::tempo);
    CHECK(sent, "unsubscribe_global should succeed");

    got = engine.wait_message_prefix("/unregister_update", msg);
    CHECK(got, "Should receive global unregister_update message");
    if (got)
    {
        CHECK_EQ(msg.path, "/unregister_update",
                 "Path must be /unregister_update");
        CHECK_EQ(msg.types, "sss", "Type signature must be sss");
    }

    /* unregister_global_auto */
    sent = client.unsubscribe_global_auto(global_control::tempo);
    CHECK(sent, "unsubscribe_global_auto should succeed");

    got = engine.wait_message_prefix("/unregister_auto_update", msg);
    CHECK(got, "Should receive global unregister_auto_update message");
    if (got)
    {
        CHECK_EQ(msg.path, "/unregister_auto_update",
                 "Path must be /unregister_auto_update");
        CHECK_EQ(msg.types, "sss",
                 "Type signature must be sss (no interval in unregister)");
    }

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 8: no receiver bound rejects subscriptions
 * ------------------------------------------------------------------------- */

static void test_no_receiver_rejects ()
{
    std::cout << "Test: no receiver bound rejects subscriptions..." << std::endl;

    sooperlooper_client client("osc.udp://127.0.0.1:9951/");

    /* No receiver bound */
    bool sent = client.subscribe_loop(0, loop_control::state);
    CHECK(! sent, "subscribe_loop without receiver should fail");

    sent = client.subscribe_loop_auto(0, loop_control::state, 50);
    CHECK(! sent, "subscribe_loop_auto without receiver should fail");

    sent = client.subscribe_global(global_control::tempo);
    CHECK(! sent, "subscribe_global without receiver should fail");

    sent = client.subscribe_global_auto(global_control::tempo, 50);
    CHECK(! sent, "subscribe_global_auto without receiver should fail");
}

/* -------------------------------------------------------------------------
 *  Test 9: cancel_subscriptions sends unregister for all tracked subs
 * ------------------------------------------------------------------------- */

static void test_cancel_subscriptions ()
{
    std::cout << "Test: cancel_subscriptions..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    /* Create two subscriptions */
    client.subscribe_loop(0, loop_control::state);
    client.subscribe_global(global_control::tempo);

    CHECK_EQ(client.active_subscription_count(), 2u,
             "Should have 2 active subscriptions");

    /* Drain the register messages so they don't interfere with counting */
    captured_message drain_msg;
    auto drain_start = std::chrono::steady_clock::now();
    while (true)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - drain_start).count();
        if (elapsed >= 500)
            break;
        engine.poll_message(drain_msg);
    }

    /* Cancel all */
    client.cancel_subscriptions();

    CHECK_EQ(client.active_subscription_count(), 0u,
             "Should have 0 active subscriptions after cancel");

    /* Should have received two unregister messages.
     * Poll all messages and count those with "unregister" in the path. */
    captured_message msg;
    int unregister_count = 0;
    auto start = std::chrono::steady_clock::now();
    while (true)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= 2000)
            break;
        if (engine.poll_message(msg))
        {
            if (msg.path.find("unregister") != std::string::npos)
                ++unregister_count;
        }
    }

    CHECK_EQ(unregister_count, 2,
             "cancel_subscriptions should send 2 unregister messages");

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 10: generation tracking
 * ------------------------------------------------------------------------- */

static void test_generation_tracking ()
{
    std::cout << "Test: generation tracking..." << std::endl;

    sooperlooper_client client;

    CHECK_EQ(client.generation(), 0u, "Initial generation must be 0");

    client.set_generation(1);
    CHECK_EQ(client.generation(), 1u, "Generation should be 1 after set");

    client.set_generation(2);
    CHECK_EQ(client.generation(), 2u, "Generation should be 2 after set");
}

/* -------------------------------------------------------------------------
 *  Test 11: callback URL and path
 * ------------------------------------------------------------------------- */

static void test_callback_url_and_path ()
{
    std::cout << "Test: callback URL and path..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");

    /* No receiver: callback_url should be empty */
    CHECK(client.callback_url().empty(),
          "callback_url should be empty without receiver");

    /* With receiver */
    client.set_receiver(&engine.receiver());
    std::string url = client.callback_url();
    CHECK(url.find("osc.udp://127.0.0.1:") == 0,
          "callback_url should start with osc.udp://127.0.0.1:");

    /* Default callback path */
    CHECK_EQ(client.callback_path(), "/reply",
             "Default callback_path must be '/reply'");

    /* Custom callback path */
    client.set_callback_path("/my/callback");
    CHECK_EQ(client.callback_path(), "/my/callback",
             "callback_path should be '/my/callback' after set");

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 12: custom callback path appears in subscription messages
 * ------------------------------------------------------------------------- */

static void test_custom_callback_path ()
{
    std::cout << "Test: custom callback path in messages..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());
    client.set_callback_path("/my/engine/reply");

    bool sent = client.subscribe_loop(0, loop_control::state);
    CHECK(sent, "subscribe_loop should succeed with custom path");

    captured_message msg;
    bool got = engine.wait_message_prefix("/sl/0/register", msg);
    CHECK(got, "Should receive message");
    if (got)
    {
        CHECK(msg.args.size() >= 3, "Must have 3 args");
        if (msg.args.size() >= 3)
        {
            CHECK_EQ(msg.args[2], "/my/engine/reply",
                     "return_path must use custom callback path");
        }
    }

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 13: shutdown sends no messages after cancel
 *  (cancel_subscriptions clears the tracking list)
 * ------------------------------------------------------------------------- */

static void test_shutdown_no_double_cancel ()
{
    std::cout << "Test: shutdown no double cancel..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    client.subscribe_loop(0, loop_control::state);
    CHECK_EQ(client.active_subscription_count(), 1u,
             "Should have 1 active subscription");

    /* First cancel */
    client.cancel_subscriptions();
    CHECK_EQ(client.active_subscription_count(), 0u,
             "Should have 0 after first cancel");

    /* Drain the unregister message */
    captured_message msg;
    engine.wait_message_prefix("/sl/0/unregister", msg, 1000);

    /* Second cancel should not send anything (list is empty) */
    client.cancel_subscriptions();
    CHECK_EQ(client.active_subscription_count(), 0u,
             "Should still have 0 after second cancel");

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 14: multiple loop subscriptions
 * ------------------------------------------------------------------------- */

static void test_multiple_loop_subscriptions ()
{
    std::cout << "Test: multiple loop subscriptions..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    /* Subscribe to different controls on different loops */
    client.subscribe_loop(0, loop_control::state);
    client.subscribe_loop(0, loop_control::loop_pos);
    client.subscribe_loop(1, loop_control::state);

    CHECK_EQ(client.active_subscription_count(), 3u,
             "Should have 3 active subscriptions");

    /* All three should be tracked */
    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 15: invalid control name rejects subscription
 * ------------------------------------------------------------------------- */

static void test_invalid_control_rejects ()
{
    std::cout << "Test: invalid control rejects subscription..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    /* Using an out-of-range enum value (cast to simulate) */
    auto bad_loop = static_cast<loop_control>(9999);
    bool sent = client.subscribe_loop(0, bad_loop);
    CHECK(! sent, "Out-of-range loop_control should be rejected");

    auto bad_global = static_cast<global_control>(9999);
    sent = client.subscribe_global(bad_global);
    CHECK(! sent, "Out-of-range global_control should be rejected");

    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Test 16: shutdown/cancel during active subscriptions
 * ------------------------------------------------------------------------- */

static void test_shutdown_during_active ()
{
    std::cout << "Test: shutdown during active subscriptions..." << std::endl;

    fake_engine engine;
    assert(engine.start());

    sooperlooper_client client("osc.udp://127.0.0.1:" +
                               std::to_string(engine.port()) + "/");
    client.set_receiver(&engine.receiver());

    /* Create several subscriptions */
    client.subscribe_loop(0, loop_control::state);
    client.subscribe_loop(1, loop_control::loop_pos);
    client.subscribe_global(global_control::tempo);
    client.subscribe_loop_auto(0, loop_control::in_peak_meter, 30);

    CHECK_EQ(client.active_subscription_count(), 4u,
             "Should have 4 active subscriptions");

    /* Simulate shutdown: cancel all */
    client.cancel_subscriptions();
    CHECK_EQ(client.active_subscription_count(), 0u,
             "All subscriptions cancelled on shutdown");

    /* Verify no crash or double-free */
    engine.stop();
}

/* -------------------------------------------------------------------------
 *  Main
 * ------------------------------------------------------------------------- */

int
main ()
{
    std::cout << "=== sooperlooper_subscription_test ===" << std::endl;

    test_subscribe_loop_wire_format();
    test_subscribe_loop_auto_wire_format();
    test_subscribe_global_wire_format();
    test_subscribe_global_auto_wire_format();
    test_interval_range_validation();
    test_loop_index_validation();
    test_unregister_wire_format();
    test_no_receiver_rejects();
    test_cancel_subscriptions();
    test_generation_tracking();
    test_callback_url_and_path();
    test_custom_callback_path();
    test_shutdown_no_double_cancel();
    test_multiple_loop_subscriptions();
    test_invalid_control_rejects();
    test_shutdown_during_active();

    std::cout << std::endl;
    if (s_failures == 0)
    {
        std::cout << "All subscription tests PASSED." << std::endl;
        return 0;
    }
    else
    {
        std::cerr << s_failures << " subscription test(s) FAILED." << std::endl;
        return 1;
    }
}

/*
 * sooperlooper_subscription_test.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
