/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_audio_slot_state_renderer_test.cpp
 *
 *  Tests for the audio slot state renderer.
 *
 *  Coverage:
 *  - Idle state rendering
 *  - Pending state rendering
 *  - Active state rendering
 *  - Stale state rendering
 *  - Offline state rendering
 *  - Failed state rendering
 *  - Indeterminate state rendering
 *  - Cached meter freshness
 *  - Display labels
 *  - Transport/record enabled states
 */

#include <iostream>
#include <string>
#include <cassert>

#include "audio/sooperlooper_audio_slot_state_renderer.hpp"

/* ------------------------------------------------------------------ */
/*  Minimal test framework                                            */
/* ------------------------------------------------------------------ */

static int g_total = 0;
static int g_failed = 0;

#define CHECK(expr) \
    do { \
        ++g_total; \
        if (! (expr)) { \
            ++g_failed; \
            std::cerr << "FAIL: " << #expr << " (line " \
                      << __LINE__ << ")\n"; \
        } \
    } while (0)

#define CHECK_EQ(a, b) \
    do { \
        ++g_total; \
        if ((a) != (b)) { \
            ++g_failed; \
            std::cerr << "FAIL: " << #a << " == " << #b \
                      << " (line " << __LINE__ << ")\n"; \
        } \
    } while (0)

/* ------------------------------------------------------------------ */
/*  Tests                                                             */
/* ------------------------------------------------------------------ */

static void
test_idle_state ()
{
    std::cout << "\n--- Idle state ---" << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    seq66::audio_slot_model model;
    auto result = renderer.render(model, 1000);
    CHECK_EQ(result.display, seq66::display_state::idle);
    CHECK_EQ(std::string(result.display_label()), "idle");
    CHECK(result.transport_enabled);
    CHECK(! result.record_enabled);
}

static void
test_pending_state ()
{
    std::cout << "\n--- Pending state ---" << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    seq66::audio_slot_model model;
    model.command_status = seq66::command_feedback::pending;
    auto result = renderer.render(model, 1000);
    CHECK_EQ(result.display, seq66::display_state::pending);
    CHECK_EQ(std::string(result.display_label()), "pending");
}

static void
test_active_state ()
{
    std::cout << "\n--- Active state ---" << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    seq66::audio_slot_model model;
    model.observed_state = seq66::loop_state::play;
    auto result = renderer.render(model, 1000);
    CHECK_EQ(result.display, seq66::display_state::active);
    CHECK_EQ(std::string(result.display_label()), "active");
}

static void
test_stale_state ()
{
    std::cout << "\n--- Stale state ---" << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    seq66::audio_slot_model model;
    model.observed_state = seq66::loop_state::play;
    model.last_state_change_ms = 1000;
    auto result = renderer.render(model, 10000);  /* 9 seconds later */
    CHECK_EQ(result.display, seq66::display_state::stale);
    CHECK_EQ(std::string(result.display_label()), "stale");
}

static void
test_offline_state ()
{
    std::cout << "\n--- Offline state ---" << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    seq66::audio_slot_model model;
    model.observed_state = seq66::loop_state::unknown;
    auto result = renderer.render(model, 1000);
    CHECK_EQ(result.display, seq66::display_state::offline);
    CHECK_EQ(std::string(result.display_label()), "offline");
}

static void
test_failed_state ()
{
    std::cout << "\n--- Failed state ---" << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    seq66::audio_slot_model model;
    model.command_status = seq66::command_feedback::failed;
    auto result = renderer.render(model, 1000);
    CHECK_EQ(result.display, seq66::display_state::failed);
    CHECK_EQ(std::string(result.display_label()), "failed");
}

static void
test_indeterminate_state ()
{
    std::cout << "\n--- Indeterminate state ---" << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    seq66::audio_slot_model model;
    model.command_status = seq66::command_feedback::indeterminate;
    auto result = renderer.render(model, 1000);
    CHECK_EQ(result.display, seq66::display_state::indeterminate);
    CHECK_EQ(std::string(result.display_label()), "indeterminate");
}

static void
test_meter_freshness ()
{
    std::cout << "\n--- Meter freshness ---" << std::endl;
    seq66::cached_meter m;
    m.present = true;
    m.timestamp_ms = 1000;
    CHECK(m.is_fresh(2000, 5000));   /* 1 second old, fresh */
    CHECK(! m.is_fresh(7000, 5000)); /* 6 seconds old, stale */
    CHECK(! m.is_fresh(2000, 500));  /* 1 second old, 500ms threshold */
}

static void
test_meter_absent ()
{
    std::cout << "\n--- Meter absent ---" << std::endl;
    seq66::cached_meter m;
    CHECK(! m.present);
    CHECK(! m.is_fresh(1000));
}

static void
test_transport_enabled ()
{
    std::cout << "\n--- Transport enabled ---" << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    seq66::audio_slot_model model;

    model.transport_playing = false;
    auto r1 = renderer.render(model, 1000);
    CHECK(r1.transport_enabled);

    model.transport_playing = true;
    auto r2 = renderer.render(model, 1000);
    CHECK(! r2.transport_enabled);
}

static void
test_record_enabled ()
{
    std::cout << "\n--- Record enabled ---" << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    seq66::audio_slot_model model;

    /* No clip: record disabled */
    auto r1 = renderer.render(model, 1000);
    CHECK(! r1.record_enabled);

    /* Has clip, not playing: record enabled */
    model.clip_uuid = "test-uuid";
    model.transport_playing = false;
    auto r2 = renderer.render(model, 1000);
    CHECK(r2.record_enabled);

    /* Has clip, playing: record disabled */
    model.transport_playing = true;
    auto r3 = renderer.render(model, 1000);
    CHECK(! r3.record_enabled);
}

static void
test_is_stale ()
{
    std::cout << "\n--- is_stale ---" << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    seq66::audio_slot_model model;

    /* No state change: not stale */
    CHECK(! renderer.is_stale(model, 1000));

    /* Recent state change: not stale */
    model.last_state_change_ms = 9000;
    CHECK(! renderer.is_stale(model, 10000, 5000));

    /* Old state change: stale */
    model.last_state_change_ms = 1000;
    CHECK(renderer.is_stale(model, 10000, 5000));
}

static void
test_is_failed ()
{
    std::cout << "\n--- is_failed ---" << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    seq66::audio_slot_model model;

    CHECK(! renderer.is_failed(model));

    model.command_status = seq66::command_feedback::failed;
    CHECK(renderer.is_failed(model));

    model.command_status = seq66::command_feedback::none;
    model.observed_state = seq66::loop_state::unknown;
    CHECK(renderer.is_failed(model));
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */

int main ()
{
    test_idle_state();
    test_pending_state();
    test_active_state();
    test_stale_state();
    test_offline_state();
    test_failed_state();
    test_indeterminate_state();
    test_meter_freshness();
    test_meter_absent();
    test_transport_enabled();
    test_record_enabled();
    test_is_stale();
    test_is_failed();

    std::cout << "\n" << g_total << "/" << g_total
              << " pass. All state renderer assertions passed." << std::endl;
    return g_failed;
}
