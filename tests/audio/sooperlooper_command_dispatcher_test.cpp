/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_command_dispatcher_test.cpp
 *
 *  Tests for the performer audio command dispatcher.
 *
 *  Covers:
 *  - command lifecycle (desired → pending → confirmed/failed/indeterminate)
 *  - deadline expiry handling
 *  - dispatch with clip mapper integration
 *  - cancel and cancel_generation
 *  - concurrent command safety
 *  - error paths and edge cases
 */

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "audio/sooperlooper_command_dispatcher.hpp"
#include "audio/sooperlooper_clip_mapper.hpp"
#include "audio/sooperlooper_command_confirmation.hpp"
#include "audio/sooperlooper_observed_state.hpp"

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

/* ------------------------------------------------------------------ */
/*  Test: initial state                                                */
/* ------------------------------------------------------------------ */

static void
test_initial_state ()
{
    std::cout << "\n--- Initial state ---" << std::endl;
    seq66::sooperlooper_command_dispatcher d;
    check(d.all_commands().empty(), "no commands");
    check(d.generation() == 0, "generation 0");
    check(d.count_by_status(seq66::command_status::desired) == 0,
          "no desired");
    check(d.count_by_status(seq66::command_status::pending) == 0,
          "no pending");
}

/* ------------------------------------------------------------------ */
/*  Test: dispatch creates desired command                             */
/* ------------------------------------------------------------------ */

static void
test_dispatch_creates_desired ()
{
    std::cout << "\n--- Dispatch creates desired ---" << std::endl;
    seq66::sooperlooper_command_dispatcher d;

    std::string uuid = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "record clip-aaa"
    );
    check(!uuid.empty(), "UUID returned");
    check(d.count_by_status(seq66::command_status::desired) == 1,
          "one desired");
    check(d.count_by_status(seq66::command_status::pending) == 0,
          "no pending");

    auto cmd = d.command_by_uuid(uuid);
    check(cmd.uuid == uuid, "UUID matches");
    check(cmd.clip_uuid == "clip-aaa", "clip UUID set");
    check(cmd.command == seq66::sooperlooper_command::record,
          "command verb set");
    check(cmd.status == seq66::command_status::desired,
          "status is desired");
}

/* ------------------------------------------------------------------ */
/*  Test: dispatch empty clip UUID fails                               */
/* ------------------------------------------------------------------ */

static void
test_dispatch_empty_uuid ()
{
    std::cout << "\n--- Dispatch empty UUID ---" << std::endl;
    seq66::sooperlooper_command_dispatcher d;

    std::string uuid = d.dispatch(
        "", seq66::sooperlooper_command::record, "fail"
    );
    check(uuid.empty(), "empty UUID returns empty string");
    check(d.all_commands().empty(), "no commands registered");
}

/* ------------------------------------------------------------------ */
/*  Test: submit resolves index and transitions to pending             */
/* ------------------------------------------------------------------ */

static void
test_submit_resolves_index ()
{
    std::cout << "\n--- Submit resolves index ---" << std::endl;
    seq66::sooperlooper_clip_mapper mapper;
    seq66::sooperlooper_command_dispatcher d;

    mapper.register_clip("clip-aaa");
    d.set_clip_mapper(&mapper);

    bool submit_called = false;
    int submit_index = -1;
    seq66::sooperlooper_command submit_cmd{};
    d.set_submit_callback(
        [&submit_called, &submit_index, &submit_cmd](
            int idx, seq66::sooperlooper_command cmd
        ) -> bool {
            submit_called = true;
            submit_index = idx;
            submit_cmd = cmd;
            return true;
        }
    );

    std::string uuid = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "record clip-aaa"
    );
    check(!uuid.empty(), "UUID returned");

    int submitted = d.submit_pending();
    check(submitted == 1, "one submitted");
    check(submit_called, "submit callback called");
    check(submit_index == 0, "resolved to index 0");
    check(submit_cmd == seq66::sooperlooper_command::record,
          "correct command verb");

    auto cmd = d.command_by_uuid(uuid);
    check(cmd.status == seq66::command_status::pending,
          "status is pending");
    check(cmd.runtime_index == 0, "runtime index resolved");
}

/* ------------------------------------------------------------------ */
/*  Test: submit without mapper leaves unresolved                      */
/* ------------------------------------------------------------------ */

static void
test_submit_without_mapper ()
{
    std::cout << "\n--- Submit without mapper ---" << std::endl;
    seq66::sooperlooper_command_dispatcher d;

    bool submit_called = false;
    d.set_submit_callback(
        [&submit_called](int, seq66::sooperlooper_command) -> bool {
            submit_called = true;
            return true;
        }
    );

    std::string uuid = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "record"
    );
    int submitted = d.submit_pending();
    check(submitted == 0, "none submitted (no mapper)");
    check(!submit_called, "submit callback not called");

    auto cmd = d.command_by_uuid(uuid);
    check(cmd.status == seq66::command_status::desired,
          "still desired");
}

/* ------------------------------------------------------------------ */
/*  Test: submit failure transitions to failed                         */
/* ------------------------------------------------------------------ */

static void
test_submit_failure ()
{
    std::cout << "\n--- Submit failure ---" << std::endl;
    seq66::sooperlooper_clip_mapper mapper;
    seq66::sooperlooper_command_dispatcher d;

    mapper.register_clip("clip-aaa");
    d.set_clip_mapper(&mapper);
    d.set_submit_callback(
        [](int, seq66::sooperlooper_command) -> bool {
            return false;
        }
    );

    std::string uuid = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "fail"
    );
    int submitted = d.submit_pending();
    check(submitted == 0, "none submitted");

    auto cmd = d.command_by_uuid(uuid);
    check(cmd.status == seq66::command_status::failed,
          "status is failed");
    check(!cmd.error.empty(), "error message set");
}

/* ------------------------------------------------------------------ */
/*  Test: confirmation via tracker                                     */
/* ------------------------------------------------------------------ */

static void
test_confirmation_via_tracker ()
{
    std::cout << "\n--- Confirmation via tracker ---" << std::endl;
    seq66::sooperlooper_clip_mapper mapper;
    seq66::command_confirmation_tracker tracker;
    seq66::sooperlooper_command_dispatcher d;

    mapper.register_clip("clip-aaa");
    d.set_clip_mapper(&mapper);
    d.set_confirmation_tracker(&tracker);

    /* Make tracker always confirm. */
    tracker.set_reconciler(
        [](const seq66::pending_operation &) -> bool {
            return true;
        }
    );

    d.set_submit_callback(
        [](int, seq66::sooperlooper_command) -> bool {
            return true;
        }
    );

    std::string uuid = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "record",
        100 /* short deadline */
    );
    d.submit_pending();

    auto cmd = d.command_by_uuid(uuid);
    check(cmd.status == seq66::command_status::pending,
          "status is pending");

    /* Wait for deadline then evaluate + reconcile. */
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    tracker.evaluate();
    tracker.reconcile();

    int resolved = d.check_feedback();
    check(resolved >= 1, "one resolved");

    cmd = d.command_by_uuid(uuid);
    check(cmd.status == seq66::command_status::confirmed,
          "status is confirmed");
}

/* ------------------------------------------------------------------ */
/*  Test: deadline expiry without tracker confirmation                 */
/* ------------------------------------------------------------------ */

static void
test_deadline_expiry ()
{
    std::cout << "\n--- Deadline expiry ---" << std::endl;
    seq66::sooperlooper_clip_mapper mapper;
    seq66::command_confirmation_tracker tracker;
    seq66::sooperlooper_command_dispatcher d;

    mapper.register_clip("clip-aaa");
    d.set_clip_mapper(&mapper);
    d.set_confirmation_tracker(&tracker);

    /* Make tracker never confirm. */
    tracker.set_reconciler(
        [](const seq66::pending_operation &) -> bool {
            return false;
        }
    );

    d.set_submit_callback(
        [](int, seq66::sooperlooper_command) -> bool {
            return true;
        }
    );

    std::string uuid = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "record",
        50 /* very short deadline */
    );
    d.submit_pending();

    /* Wait for deadline. */
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    /* Evaluate tracker deadlines. */
    tracker.evaluate();

    int resolved = d.check_feedback();
    check(resolved >= 1, "one resolved");

    auto cmd = d.command_by_uuid(uuid);
    check(cmd.status == seq66::command_status::indeterminate,
          "status is indeterminate");
}

/* ------------------------------------------------------------------ */
/*  Test: cancel command                                               */
/* ------------------------------------------------------------------ */

static void
test_cancel ()
{
    std::cout << "\n--- Cancel ---" << std::endl;
    seq66::sooperlooper_command_dispatcher d;

    std::string uuid = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "record"
    );
    check(d.count_by_status(seq66::command_status::desired) == 1,
          "one desired");

    bool cancelled = d.cancel(uuid);
    check(cancelled, "cancel returned true");
    check(d.count_by_status(seq66::command_status::desired) == 0,
          "no desired after cancel");
    check(d.count_by_status(seq66::command_status::cancelled) == 1,
          "one cancelled");
}

/* ------------------------------------------------------------------ */
/*  Test: cancel unknown UUID                                          */
/* ------------------------------------------------------------------ */

static void
test_cancel_unknown ()
{
    std::cout << "\n--- Cancel unknown ---" << std::endl;
    seq66::sooperlooper_command_dispatcher d;

    bool cancelled = d.cancel("nonexistent");
    check(!cancelled, "cancel unknown returns false");
}

/* ------------------------------------------------------------------ */
/*  Test: cancel already confirmed fails                               */
/* ------------------------------------------------------------------ */

static void
test_cancel_confirmed_fails ()
{
    std::cout << "\n--- Cancel confirmed fails ---" << std::endl;
    seq66::sooperlooper_clip_mapper mapper;
    seq66::command_confirmation_tracker tracker;
    seq66::sooperlooper_command_dispatcher d;

    mapper.register_clip("clip-aaa");
    d.set_clip_mapper(&mapper);
    d.set_confirmation_tracker(&tracker);
    tracker.set_reconciler(
        [](const seq66::pending_operation &) -> bool { return true; }
    );
    d.set_submit_callback(
        [](int, seq66::sooperlooper_command) -> bool { return true; }
    );

    std::string uuid = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "record", 50
    );
    d.submit_pending();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    tracker.evaluate();
    tracker.reconcile();
    d.check_feedback();

    auto cmd = d.command_by_uuid(uuid);
    check(cmd.status == seq66::command_status::confirmed,
          "confirmed");

    bool cancelled = d.cancel(uuid);
    check(!cancelled, "cancel confirmed returns false");
    check(d.count_by_status(seq66::command_status::confirmed) == 1,
          "still confirmed");
}

/* ------------------------------------------------------------------ */
/*  Test: cancel_generation                                            */
/* ------------------------------------------------------------------ */

static void
test_cancel_generation ()
{
    std::cout << "\n--- Cancel generation ---" << std::endl;
    seq66::sooperlooper_clip_mapper mapper;
    seq66::sooperlooper_command_dispatcher d;

    mapper.register_clip("clip-aaa");
    mapper.register_clip("clip-bbb");
    d.set_clip_mapper(&mapper);

    d.set_submit_callback(
        [](int, seq66::sooperlooper_command) -> bool { return true; }
    );

    /* Dispatch two commands in generation 0. */
    d.set_generation(0);
    std::string uuid1 = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "rec A"
    );
    std::string uuid2 = d.dispatch(
        "clip-bbb", seq66::sooperlooper_command::overdub, "dub B"
    );
    d.submit_pending();

    check(d.count_by_status(seq66::command_status::pending) == 2,
          "two pending");

    /* Advance generation and cancel old. */
    d.set_generation(1);
    int cancelled = d.cancel_generation(0);
    check(cancelled == 2, "two cancelled");

    auto cmd1 = d.command_by_uuid(uuid1);
    auto cmd2 = d.command_by_uuid(uuid2);
    check(cmd1.status == seq66::command_status::cancelled,
          "cmd1 cancelled");
    check(cmd2.status == seq66::command_status::cancelled,
          "cmd2 cancelled");
}

/* ------------------------------------------------------------------ */
/*  Test: status callback                                              */
/* ------------------------------------------------------------------ */

static void
test_status_callback ()
{
    std::cout << "\n--- Status callback ---" << std::endl;
    seq66::sooperlooper_clip_mapper mapper;
    seq66::sooperlooper_command_dispatcher d;

    mapper.register_clip("clip-aaa");
    d.set_clip_mapper(&mapper);

    int transition_count = 0;
    seq66::command_status last_status{};
    d.set_status_callback(
        [&transition_count, &last_status](
            const seq66::audio_command & cmd
        ) {
            ++transition_count;
            last_status = cmd.status;
        }
    );

    d.set_submit_callback(
        [](int, seq66::sooperlooper_command) -> bool { return true; }
    );

    d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "record"
    );
    /* dispatch doesn't trigger callback (no transition). */
    check(transition_count == 0, "no callback on dispatch");

    d.submit_pending();
    /* submit triggers desired→pending. */
    check(transition_count == 1, "one callback on submit");
    check(last_status == seq66::command_status::pending,
          "callback status is pending");
}

/* ------------------------------------------------------------------ */
/*  Test: multiple commands lifecycle                                  */
/* ------------------------------------------------------------------ */

static void
test_multiple_commands ()
{
    std::cout << "\n--- Multiple commands ---" << std::endl;
    seq66::sooperlooper_clip_mapper mapper;
    seq66::command_confirmation_tracker tracker;
    seq66::sooperlooper_command_dispatcher d;

    mapper.register_clip("clip-aaa");
    mapper.register_clip("clip-bbb");
    mapper.register_clip("clip-ccc");
    d.set_clip_mapper(&mapper);
    d.set_confirmation_tracker(&tracker);
    tracker.set_reconciler(
        [](const seq66::pending_operation &) -> bool { return false; }
    );
    d.set_submit_callback(
        [](int, seq66::sooperlooper_command) -> bool { return true; }
    );

    d.dispatch("clip-aaa", seq66::sooperlooper_command::record, "rec A");
    d.dispatch("clip-bbb", seq66::sooperlooper_command::overdub, "dub B");
    d.dispatch("clip-ccc", seq66::sooperlooper_command::mute, "mute C");

    check(d.count_by_status(seq66::command_status::desired) == 3,
          "three desired");

    d.submit_pending();
    check(d.count_by_status(seq66::command_status::pending) == 3,
          "three pending");

    /* Cancel one. */
    auto all = d.all_commands();
    d.cancel(all[1].uuid);
    check(d.count_by_status(seq66::command_status::cancelled) == 1,
          "one cancelled");
    check(d.count_by_status(seq66::command_status::pending) == 2,
          "two pending");
}

/* ------------------------------------------------------------------ */
/*  Test: clear                                                        */
/* ------------------------------------------------------------------ */

static void
test_clear ()
{
    std::cout << "\n--- Clear ---" << std::endl;
    seq66::sooperlooper_command_dispatcher d;

    d.dispatch("clip-aaa", seq66::sooperlooper_command::record, "rec");
    d.dispatch("clip-bbb", seq66::sooperlooper_command::overdub, "dub");
    check(d.all_commands().size() == 2, "two commands");

    d.clear();
    check(d.all_commands().empty(), "no commands after clear");
}

/* ------------------------------------------------------------------ */
/*  Test: submit_pending with no submit callback                       */
/* ------------------------------------------------------------------ */

static void
test_submit_no_callback ()
{
    std::cout << "\n--- Submit no callback ---" << std::endl;
    seq66::sooperlooper_clip_mapper mapper;
    seq66::sooperlooper_command_dispatcher d;

    mapper.register_clip("clip-aaa");
    d.set_clip_mapper(&mapper);
    /* No submit callback set. */

    std::string uuid = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "record"
    );
    int submitted = d.submit_pending();
    check(submitted == 0, "none submitted (no callback)");

    auto cmd = d.command_by_uuid(uuid);
    check(cmd.status == seq66::command_status::desired,
          "still desired");
}

/* ------------------------------------------------------------------ */
/*  Test: multiple dispatches accumulate                               */
/* ------------------------------------------------------------------ */

static void
test_multiple_dispatches ()
{
    std::cout << "\n--- Multiple dispatches ---" << std::endl;
    seq66::sooperlooper_command_dispatcher d;

    std::string u1 = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::record, "rec A"
    );
    std::string u2 = d.dispatch(
        "clip-aaa", seq66::sooperlooper_command::overdub, "dub A"
    );
    std::string u3 = d.dispatch(
        "clip-bbb", seq66::sooperlooper_command::mute, "mute B"
    );

    check(!u1.empty() && !u2.empty() && !u3.empty(), "all UUIDs set");
    check(u1 != u2, "UUIDs unique");
    check(u1 != u3, "UUIDs unique (cross)");
    check(d.all_commands().size() == 3, "three commands");
}

/* ------------------------------------------------------------------ */
/*  Test: dispatch with mapper miss                                    */
/* ------------------------------------------------------------------ */

static void
test_dispatch_mapper_miss ()
{
    std::cout << "\n--- Dispatch mapper miss ---" << std::endl;
    seq66::sooperlooper_clip_mapper mapper;
    seq66::sooperlooper_command_dispatcher d;

    mapper.register_clip("clip-aaa");
    d.set_clip_mapper(&mapper);

    bool submit_called = false;
    d.set_submit_callback(
        [&submit_called](int, seq66::sooperlooper_command) -> bool {
            submit_called = true;
            return true;
        }
    );

    /* Dispatch a clip that doesn't exist in the mapper. */
    std::string uuid = d.dispatch(
        "nonexistent", seq66::sooperlooper_command::record, "fail"
    );
    int submitted = d.submit_pending();
    check(submitted == 0, "none submitted (UUID not in mapper)");
    check(!submit_called, "submit callback not called");
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int
main ()
{
    std::cout << "=== Command dispatcher tests ===" << std::endl;

    test_initial_state();
    test_dispatch_creates_desired();
    test_dispatch_empty_uuid();
    test_submit_resolves_index();
    test_submit_without_mapper();
    test_submit_failure();
    test_confirmation_via_tracker();
    test_deadline_expiry();
    test_cancel();
    test_cancel_unknown();
    test_cancel_confirmed_fails();
    test_cancel_generation();
    test_status_callback();
    test_multiple_commands();
    test_clear();
    test_submit_no_callback();
    test_multiple_dispatches();
    test_dispatch_mapper_miss();

    std::cout << "\n" << g_assertions << " assertions, "
              << g_failures << " failures." << std::endl;
    return g_failures > 0 ? 1 : 0;
}
