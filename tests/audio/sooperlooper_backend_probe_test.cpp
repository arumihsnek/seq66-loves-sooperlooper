/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_backend_probe_test.cpp
 *
 *  Tests for the backend capability probe.  Uses injectable fake detection
 *  functions to verify all probe paths without requiring real JACK libraries.
 *
 *  Build command (mirrors audio-core.yml):
 *      g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -pthread \
 *          -Ilibseq66/include \
 *          libseq66/src/audio/sooperlooper_backend_probe.cpp \
 *          tests/audio/sooperlooper_backend_probe_test.cpp \
 *          -o .ci/bin/sooperlooper_backend_probe_test
 */

#include "audio/sooperlooper_backend_probe.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>

static int s_failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (! (cond)) { \
            std::cerr << "  [FAIL] " << msg << std::endl; \
            ++s_failures; \
        } else { \
            std::cout << "  [PASS] " << msg << std::endl; \
        } \
    } while (0)

using backend_probe_fn = std::function<seq66::backend_probe_result()>;

/* ------------------------------------------------------------------ */
/*  Fake detection functions                                           */
/* ------------------------------------------------------------------ */

static seq66::backend_probe_result
fake_native_jack ()
{
    seq66::backend_probe_result r;
    r.capability = seq66::backend_capability::usable_native_jack;
    r.client_library = "jack";
    r.implementation = "native";
    r.server_info = "JACK 1.9.12";
    r.server_reachable = true;
    return r;
}

static seq66::backend_probe_result
fake_pipewire_jack ()
{
    seq66::backend_probe_result r;
    r.capability = seq66::backend_capability::usable_pipewire_jack;
    r.client_library = "jack";
    r.implementation = "pipewire";
    r.server_info = "PipeWire 0.3.48";
    r.server_reachable = true;
    return r;
}

static seq66::backend_probe_result
fake_alsa_only ()
{
    seq66::backend_probe_result r;
    r.capability = seq66::backend_capability::backend_unavailable;
    r.error = "no JACK library found";
    return r;
}

static seq66::backend_probe_result
fake_probe_error ()
{
    seq66::backend_probe_result r;
    r.capability = seq66::backend_capability::probe_error;
    r.error = "dlopen failed: libjack.so not found";
    return r;
}

static seq66::backend_probe_result
fake_no_server ()
{
    seq66::backend_probe_result r;
    r.capability = seq66::backend_capability::backend_unavailable;
    r.error = "JACK server not reachable";
    r.server_reachable = false;
    return r;
}

/* ------------------------------------------------------------------ */
/*  Test groups                                                        */
/* ------------------------------------------------------------------ */

static void
test_native_jack_classification ()
{
    std::cout << "\n--- Native JACK classification ---" << std::endl;
    auto result = seq66::probe_backend_capability(fake_native_jack);
    CHECK(result.capability == seq66::backend_capability::usable_native_jack,
          "capability is usable_native_jack");
    CHECK(result.is_usable(), "is_usable returns true");
    CHECK(! result.is_unavailable(), "is_unavailable returns false");
    CHECK(! result.has_error(), "has_error returns false");
    CHECK(result.implementation == "native", "implementation is native");
    CHECK(result.client_library == "jack", "client_library is jack");
    CHECK(result.server_reachable, "server_reachable is true");
}

static void
test_pipewire_jack_classification ()
{
    std::cout << "\n--- PipeWire-JACK classification ---" << std::endl;
    auto result = seq66::probe_backend_capability(fake_pipewire_jack);
    CHECK(result.capability == seq66::backend_capability::usable_pipewire_jack,
          "capability is usable_pipewire_jack");
    CHECK(result.is_usable(), "is_usable returns true");
    CHECK(! result.is_unavailable(), "is_unavailable returns false");
    CHECK(result.implementation == "pipewire", "implementation is pipewire");
    CHECK(result.server_reachable, "server_reachable is true");
}

static void
test_alsa_only_unavailable ()
{
    std::cout << "\n--- ALSA-only backend_unavailable ---" << std::endl;
    auto result = seq66::probe_backend_capability(fake_alsa_only);
    CHECK(result.capability == seq66::backend_capability::backend_unavailable,
          "capability is backend_unavailable");
    CHECK(! result.is_usable(), "is_usable returns false");
    CHECK(result.is_unavailable(), "is_unavailable returns true");
    CHECK(! result.has_error(), "has_error returns false");
    CHECK(! result.error.empty(), "error message is present");
}

static void
test_probe_error ()
{
    std::cout << "\n--- Probe error ---" << std::endl;
    auto result = seq66::probe_backend_capability(fake_probe_error);
    CHECK(result.capability == seq66::backend_capability::probe_error,
          "capability is probe_error");
    CHECK(! result.is_usable(), "is_usable returns false");
    CHECK(! result.is_unavailable(), "is_unavailable returns false");
    CHECK(result.has_error(), "has_error returns true");
    CHECK(! result.error.empty(), "error message is present");
}

static void
test_no_server ()
{
    std::cout << "\n--- JACK library present but no server ---" << std::endl;
    auto result = seq66::probe_backend_capability(fake_no_server);
    CHECK(result.capability == seq66::backend_capability::backend_unavailable,
          "capability is backend_unavailable");
    CHECK(! result.server_reachable, "server_reachable is false");
}

static void
test_to_string_capability ()
{
    std::cout << "\n--- to_string capability ---" << std::endl;
    CHECK(std::string(seq66::to_string(seq66::backend_capability::usable_native_jack))
              == "usable_native_jack",
          "to_string(usable_native_jack)");
    CHECK(std::string(seq66::to_string(seq66::backend_capability::usable_pipewire_jack))
              == "usable_pipewire_jack",
          "to_string(usable_pipewire_jack)");
    CHECK(std::string(seq66::to_string(seq66::backend_capability::backend_unavailable))
              == "backend_unavailable",
          "to_string(backend_unavailable)");
    CHECK(std::string(seq66::to_string(seq66::backend_capability::probe_error))
              == "probe_error",
          "to_string(probe_error)");
    CHECK(std::string(seq66::to_string(seq66::backend_capability::unknown))
              == "unknown",
          "to_string(unknown)");
}

static void
test_to_string_result ()
{
    std::cout << "\n--- to_string result ---" << std::endl;
    auto result = fake_native_jack();
    auto s = seq66::to_string(result);
    CHECK(s.find("usable_native_jack") != std::string::npos,
          "result string contains capability");
    CHECK(s.find("native") != std::string::npos,
          "result string contains implementation");
}

static void
test_default_probe ()
{
    std::cout << "\n--- Default system probe ---" << std::endl;
    auto result = seq66::probe_backend_capability(nullptr);
    /* Default probe returns either usable or unavailable depending on env */
    CHECK(result.capability != seq66::backend_capability::unknown,
          "default probe returns a known capability");
    CHECK(result.capability != seq66::backend_capability::probe_error,
          "default probe does not return probe_error in normal env");
}

static void
test_nullptr_uses_default ()
{
    std::cout << "\n--- nullptr uses default probe ---" << std::endl;
    auto result = seq66::probe_backend_capability();
    CHECK(result.capability != seq66::backend_capability::unknown,
          "default probe returns a known capability");
}

/* ------------------------------------------------------------------ */
/*  Main                                                               */
/* ------------------------------------------------------------------ */

int
main ()
{
    std::cout << "=== Backend capability probe tests ===" << std::endl;

    test_native_jack_classification();
    test_pipewire_jack_classification();
    test_alsa_only_unavailable();
    test_probe_error();
    test_no_server();
    test_to_string_capability();
    test_to_string_result();
    test_default_probe();
    test_nullptr_uses_default();

    std::cout << std::endl;
    if (s_failures == 0)
    {
        std::cout << "All backend probe tests passed." << std::endl;
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
