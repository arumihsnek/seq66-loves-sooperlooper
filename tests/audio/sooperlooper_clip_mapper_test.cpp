/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_clip_mapper_test.cpp
 *
 *  Tests for the clip UUID to runtime-index mapper.
 */

#include <iostream>
#include <string>

#include "audio/sooperlooper_clip_mapper.hpp"

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

static void
test_initial_state ()
{
    std::cout << "\n--- Initial state ---" << std::endl;
    seq66::sooperlooper_clip_mapper m;
    check(m.clip_count() == 0, "no clips");
    check(m.active_count() == 0, "no active clips");
    check(m.generation() == 0, "generation 0");
}

static void
test_register_and_lookup ()
{
    std::cout << "\n--- Register and lookup ---" << std::endl;
    seq66::sooperlooper_clip_mapper m;

    int idx1 = m.register_clip("uuid-aaa");
    check(idx1 == 0, "first clip gets index 0");
    check(m.clip_count() == 1, "one clip registered");
    check(m.is_registered("uuid-aaa"), "uuid-aaa registered");

    int idx2 = m.register_clip("uuid-bbb");
    check(idx2 == 1, "second clip gets index 1");
    check(m.clip_count() == 2, "two clips registered");

    check(m.lookup("uuid-aaa") == 0, "lookup uuid-aaa returns 0");
    check(m.lookup("uuid-bbb") == 1, "lookup uuid-bbb returns 1");
    check(m.reverse_lookup(0) == "uuid-aaa", "reverse 0 returns uuid-aaa");
    check(m.reverse_lookup(1) == "uuid-bbb", "reverse 1 returns uuid-bbb");
}

static void
test_duplicate_rejected ()
{
    std::cout << "\n--- Duplicate rejected ---" << std::endl;
    seq66::sooperlooper_clip_mapper m;

    m.register_clip("uuid-aaa");
    int dup = m.register_clip("uuid-aaa");
    check(dup == -1, "duplicate returns -1");
    check(m.clip_count() == 1, "still one clip");
}

static void
test_unregister ()
{
    std::cout << "\n--- Unregister ---" << std::endl;
    seq66::sooperlooper_clip_mapper m;

    m.register_clip("uuid-aaa");
    m.register_clip("uuid-bbb");
    m.register_clip("uuid-ccc");
    check(m.clip_count() == 3, "three clips");

    bool removed = m.unregister_clip("uuid-bbb");
    check(removed, "uuid-bbb removed");
    check(m.clip_count() == 2, "two clips after remove");
    check(!m.is_registered("uuid-bbb"), "uuid-bbb not registered");
    check(m.lookup("uuid-bbb") == -1, "uuid-bbb lookup returns -1");

    /* Remaining clips still accessible. */
    check(m.lookup("uuid-aaa") >= 0, "uuid-aaa still accessible");
    check(m.lookup("uuid-ccc") >= 0, "uuid-ccc still accessible");
}

static void
test_generation_invalidation ()
{
    std::cout << "\n--- Generation invalidation ---" << std::endl;
    seq66::sooperlooper_clip_mapper m;

    m.register_clip("uuid-aaa");
    m.register_clip("uuid-bbb");
    check(m.active_count() == 2, "two active clips");

    m.invalidate_generation(0);
    check(m.active_count() == 0, "no active after invalidation");
    check(m.lookup("uuid-aaa") == -1, "uuid-aaa lookup returns -1");
    check(m.clip_count() == 2, "clips still registered");
}

static void
test_advance_generation ()
{
    std::cout << "\n--- Advance generation ---" << std::endl;
    seq66::sooperlooper_clip_mapper m;

    m.register_clip("uuid-aaa");
    m.register_clip("uuid-bbb");

    /* Invalidate gen 0. */
    m.invalidate_generation(0);
    check(m.active_count() == 0, "no active after invalidate");

    /* Advance to gen 1. */
    int remapped = m.advance_generation(1);
    check(remapped == 2, "two clips remapped");
    check(m.generation() == 1, "generation is 1");
    check(m.active_count() == 2, "two active after advance");
    check(m.lookup("uuid-aaa") >= 0, "uuid-aaa accessible in gen 1");
    check(m.lookup("uuid-bbb") >= 0, "uuid-bbb accessible in gen 1");
}

static void
test_stable_uuid_across_restarts ()
{
    std::cout << "\n--- Stable UUID across restarts ---" << std::endl;
    seq66::sooperlooper_clip_mapper m;

    /* Simulate first session. */
    m.register_clip("persistent-uuid");
    int idx1 = m.lookup("persistent-uuid");
    check(idx1 == 0, "index 0 in first session");

    /* Simulate restart: invalidate and advance. */
    m.invalidate_generation(0);
    m.advance_generation(1);
    int idx2 = m.lookup("persistent-uuid");
    check(idx2 >= 0, "uuid accessible after restart");
    check(m.is_registered("persistent-uuid"), "uuid still registered");
}

static void
test_crash_invalidation ()
{
    std::cout << "\n--- Crash invalidation ---" << std::endl;
    seq66::sooperlooper_clip_mapper m;

    m.register_clip("uuid-aaa");
    m.register_clip("uuid-bbb");
    check(m.active_count() == 2, "two active before crash");

    /* Simulate crash: invalidate current generation. */
    m.invalidate_generation(0);
    check(m.active_count() == 0, "no active after crash");
    check(m.lookup("uuid-aaa") == -1, "index invalidated");
    check(m.clip_count() == 2, "clips preserved (not deleted)");
}

static void
test_callback_on_invalidation ()
{
    std::cout << "\n--- Callback on invalidation ---" << std::endl;
    seq66::sooperlooper_clip_mapper m;

    bool callback_called = false;
    std::uint64_t callback_gen = 0;
    m.set_on_invalidate([&callback_called, &callback_gen](std::uint64_t gen)
    {
        callback_called = true;
        callback_gen = gen;
    });

    m.register_clip("uuid-aaa");
    m.invalidate_generation(42);
    check(callback_called, "callback called");
    check(callback_gen == 42, "callback received correct generation");
}

static void
test_clear ()
{
    std::cout << "\n--- Clear ---" << std::endl;
    seq66::sooperlooper_clip_mapper m;

    m.register_clip("uuid-aaa");
    m.register_clip("uuid-bbb");
    m.clear();
    check(m.clip_count() == 0, "no clips after clear");
    check(m.generation() == 0, "generation reset to 0");
    check(!m.is_registered("uuid-aaa"), "uuid-aaa not registered");
}

int
main ()
{
    std::cout << "=== Clip mapper tests ===" << std::endl;

    test_initial_state();
    test_register_and_lookup();
    test_duplicate_rejected();
    test_unregister();
    test_generation_invalidation();
    test_advance_generation();
    test_stable_uuid_across_restarts();
    test_crash_invalidation();
    test_callback_on_invalidation();
    test_clear();

    std::cout << "\n" << g_assertions << " assertions, "
              << g_failures << " failures." << std::endl;
    return g_failures > 0 ? 1 : 0;
}
