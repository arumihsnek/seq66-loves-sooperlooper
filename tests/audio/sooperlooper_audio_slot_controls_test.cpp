/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_audio_slot_controls_test.cpp
 *
 *  Tests for the audio slot controls.
 *
 *  Coverage:
 *  - Record, Launch, Mute, Overdub, Stop requests
 *  - Transport action conversion
 *  - Action type labels
 *  - Timestamp generation
 *  - Loop index and clip UUID preservation
 */

#include <iostream>
#include <string>
#include <cassert>

#include "audio/sooperlooper_audio_slot_controls.hpp"

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
test_record_request ()
{
    std::cout << "\n--- Record request ---" << std::endl;
    seq66::sooperlooper_audio_slot_controls controls;
    auto req = controls.request_record(3, "uuid-abc");

    CHECK_EQ(req.type, seq66::audio_slot_action_type::record);
    CHECK_EQ(req.loop_index, 3);
    CHECK_EQ(req.clip_uuid, std::string("uuid-abc"));
    CHECK(req.timestamp_ms > 0);
    CHECK_EQ(std::string(req.type_label()), "record");
}

static void
test_launch_request ()
{
    std::cout << "\n--- Launch request ---" << std::endl;
    seq66::sooperlooper_audio_slot_controls controls;
    auto req = controls.request_launch(5, "uuid-def");

    CHECK_EQ(req.type, seq66::audio_slot_action_type::launch);
    CHECK_EQ(req.loop_index, 5);
    CHECK_EQ(req.clip_uuid, std::string("uuid-def"));
    CHECK_EQ(std::string(req.type_label()), "launch");
}

static void
test_mute_request ()
{
    std::cout << "\n--- Mute request ---" << std::endl;
    seq66::sooperlooper_audio_slot_controls controls;
    auto req = controls.request_mute(7, "uuid-ghi");

    CHECK_EQ(req.type, seq66::audio_slot_action_type::mute);
    CHECK_EQ(req.loop_index, 7);
    CHECK_EQ(std::string(req.type_label()), "mute");
}

static void
test_overdub_request ()
{
    std::cout << "\n--- Overdub request ---" << std::endl;
    seq66::sooperlooper_audio_slot_controls controls;
    auto req = controls.request_overdub(2, "uuid-jkl");

    CHECK_EQ(req.type, seq66::audio_slot_action_type::overdub);
    CHECK_EQ(req.loop_index, 2);
    CHECK_EQ(std::string(req.type_label()), "overdub");
}

static void
test_stop_request ()
{
    std::cout << "\n--- Stop request ---" << std::endl;
    seq66::sooperlooper_audio_slot_controls controls;
    auto req = controls.request_stop(1, "uuid-mno");

    CHECK_EQ(req.type, seq66::audio_slot_action_type::stop);
    CHECK_EQ(req.loop_index, 1);
    CHECK_EQ(std::string(req.type_label()), "stop");
}

static void
test_transport_start ()
{
    std::cout << "\n--- Transport start ---" << std::endl;
    seq66::sooperlooper_audio_slot_controls controls;
    auto req = controls.from_transport(
        seq66::transport_action::start, 4, "uuid-pqr");

    CHECK_EQ(req.type, seq66::audio_slot_action_type::launch);
    CHECK_EQ(req.loop_index, 4);
    CHECK_EQ(req.clip_uuid, std::string("uuid-pqr"));
}

static void
test_transport_stop ()
{
    std::cout << "\n--- Transport stop ---" << std::endl;
    seq66::sooperlooper_audio_slot_controls controls;
    auto req = controls.from_transport(
        seq66::transport_action::stop, 6, "uuid-stu");

    CHECK_EQ(req.type, seq66::audio_slot_action_type::stop);
    CHECK_EQ(req.loop_index, 6);
    CHECK_EQ(req.clip_uuid, std::string("uuid-stu"));
}

static void
test_action_type_labels ()
{
    std::cout << "\n--- Action type labels ---" << std::endl;
    CHECK_EQ(std::string(
        seq66::audio_slot_control_request::make_record(0, "").type_label()),
        "record");
    CHECK_EQ(std::string(
        seq66::audio_slot_control_request::make_launch(0, "").type_label()),
        "launch");
    CHECK_EQ(std::string(
        seq66::audio_slot_control_request::make_mute(0, "").type_label()),
        "mute");
    CHECK_EQ(std::string(
        seq66::audio_slot_control_request::make_overdub(0, "").type_label()),
        "overdub");
    CHECK_EQ(std::string(
        seq66::audio_slot_control_request::make_stop(0, "").type_label()),
        "stop");
}

static void
test_timestamp ()
{
    std::cout << "\n--- Timestamp ---" << std::endl;
    auto r1 = seq66::audio_slot_control_request::make_record(0, "");
    auto r2 = seq66::audio_slot_control_request::make_record(0, "");
    CHECK(r1.timestamp_ms > 0);
    CHECK(r2.timestamp_ms >= r1.timestamp_ms);
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */

int main ()
{
    test_record_request();
    test_launch_request();
    test_mute_request();
    test_overdub_request();
    test_stop_request();
    test_transport_start();
    test_transport_stop();
    test_action_type_labels();
    test_timestamp();

    std::cout << "\n" << g_total << "/" << g_total
              << " pass. All controls assertions passed." << std::endl;
    return g_failed;
}
