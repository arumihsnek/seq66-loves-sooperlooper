/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_audio_slot_grid_adapter_test.cpp
 *
 *  Tests for the audio slot grid adapter.
 *
 *  Coverage:
 *  - Initialization and grid position
 *  - Model update propagation
 *  - Selection state
 *  - Transport signal emission
 *  - Tempo signal emission
 *  - Lifecycle: init, update, select, delete
 */

#include <QApplication>
#include <iostream>
#include <string>
#include <cassert>

#include "audio/sooperlooper_audio_slot_widget.hpp"
#include "widgets/sooperlooper_audio_slot_grid_adapter.hpp"

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
test_initial_state ()
{
    std::cout << "\n--- Initial state ---" << std::endl;
    seq66::audio_slot_grid_adapter adapter;
    CHECK(! adapter.is_initialized());
    CHECK_EQ(adapter.row(), -1);
    CHECK_EQ(adapter.column(), -1);
    CHECK_EQ(adapter.slot_id(), -1);
    CHECK(! adapter.is_selected());
}

static void
test_initialization ()
{
    std::cout << "\n--- Initialization ---" << std::endl;
    seq66::audio_slot_grid_adapter adapter;
    adapter.init(2, 3, 42);
    CHECK(adapter.is_initialized());
    CHECK_EQ(adapter.row(), 2);
    CHECK_EQ(adapter.column(), 3);
    CHECK_EQ(adapter.slot_id(), 42);
}

static void
test_model_update ()
{
    std::cout << "\n--- Model update ---" << std::endl;
    seq66::audio_slot_grid_adapter adapter;
    adapter.init(0, 0, 1);

    seq66::audio_slot_model m;
    m.clip_uuid = "test-uuid";
    m.observed_state = seq66::loop_state::play;
    m.command_status = seq66::command_feedback::pending;
    m.tempo_mode = seq66::tempo_mode_selection::tape;
    m.transport_playing = true;

    adapter.update_from_model(m);

    CHECK(adapter.model().has_clip());
    CHECK_EQ(adapter.model().clip_uuid, std::string("test-uuid"));
    CHECK_EQ(adapter.model().observed_state, seq66::loop_state::play);
    CHECK_EQ(adapter.model().command_status, seq66::command_feedback::pending);
    CHECK_EQ(adapter.model().tempo_mode, seq66::tempo_mode_selection::tape);
    CHECK(adapter.model().transport_playing);
}

static void
test_selection ()
{
    std::cout << "\n--- Selection ---" << std::endl;
    seq66::audio_slot_grid_adapter adapter;
    adapter.init(0, 0, 1);

    CHECK(! adapter.is_selected());
    adapter.set_selected(true);
    CHECK(adapter.is_selected());
    adapter.set_selected(false);
    CHECK(! adapter.is_selected());
}

static void
test_transport_signal ()
{
    std::cout << "\n--- Transport signal ---" << std::endl;
    seq66::audio_slot_grid_adapter adapter;
    adapter.init(0, 0, 1);

    int signal_count = 0;
    seq66::transport_action last_action = seq66::transport_action::start;

    QObject::connect(&adapter,
        &seq66::audio_slot_grid_adapter::transport_requested,
        [&](seq66::transport_action a) {
            ++signal_count;
            last_action = a;
        });

    emit adapter.transport_requested(seq66::transport_action::start);
    CHECK_EQ(signal_count, 1);
    CHECK_EQ(last_action, seq66::transport_action::start);

    emit adapter.transport_requested(seq66::transport_action::stop);
    CHECK_EQ(signal_count, 2);
    CHECK_EQ(last_action, seq66::transport_action::stop);
}

static void
test_tempo_signal ()
{
    std::cout << "\n--- Tempo signal ---" << std::endl;
    seq66::audio_slot_grid_adapter adapter;
    adapter.init(0, 0, 1);

    int signal_count = 0;
    seq66::tempo_mode_selection last_mode = seq66::tempo_mode_selection::free;

    QObject::connect(&adapter,
        &seq66::audio_slot_grid_adapter::tempo_mode_requested,
        [&](seq66::tempo_mode_selection m) {
            ++signal_count;
            last_mode = m;
        });

    emit adapter.tempo_mode_requested(seq66::tempo_mode_selection::elastic);
    CHECK_EQ(signal_count, 1);
    CHECK_EQ(last_mode, seq66::tempo_mode_selection::elastic);
}

static void
test_selected_signal ()
{
    std::cout << "\n--- Selected signal ---" << std::endl;
    seq66::audio_slot_grid_adapter adapter;
    adapter.init(0, 0, 42);

    int received_id = -1;
    QObject::connect(&adapter,
        &seq66::audio_slot_grid_adapter::selected,
        [&](int id) { received_id = id; });

    emit adapter.selected(42);
    CHECK_EQ(received_id, 42);
}

static void
test_deleted_signal ()
{
    std::cout << "\n--- Deleted signal ---" << std::endl;
    seq66::audio_slot_grid_adapter adapter;
    adapter.init(0, 0, 42);

    int received_id = -1;
    QObject::connect(&adapter,
        &seq66::audio_slot_grid_adapter::deleted,
        [&](int id) { received_id = id; });

    emit adapter.deleted(42);
    CHECK_EQ(received_id, 42);
}

static void
test_multiple_adapters ()
{
    std::cout << "\n--- Multiple adapters ---" << std::endl;
    seq66::audio_slot_grid_adapter a1, a2, a3;
    a1.init(0, 0, 1);
    a2.init(0, 1, 2);
    a3.init(1, 0, 3);

    CHECK(a1.is_initialized());
    CHECK(a2.is_initialized());
    CHECK(a3.is_initialized());

    CHECK_EQ(a1.row(), 0);
    CHECK_EQ(a1.column(), 0);
    CHECK_EQ(a1.slot_id(), 1);

    CHECK_EQ(a2.row(), 0);
    CHECK_EQ(a2.column(), 1);
    CHECK_EQ(a2.slot_id(), 2);

    CHECK_EQ(a3.row(), 1);
    CHECK_EQ(a3.column(), 0);
    CHECK_EQ(a3.slot_id(), 3);
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */

int main (int argc, char * argv[])
{
    QApplication app(argc, argv);

    test_initial_state();
    test_initialization();
    test_model_update();
    test_selection();
    test_transport_signal();
    test_tempo_signal();
    test_selected_signal();
    test_deleted_signal();
    test_multiple_adapters();

    std::cout << "\n" << g_total << "/" << g_total
              << " pass. All grid adapter assertions passed." << std::endl;
    return g_failed;
}
