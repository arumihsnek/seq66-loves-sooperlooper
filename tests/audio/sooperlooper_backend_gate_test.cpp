/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 * \file          sooperlooper_backend_gate_test.cpp
 *
 *  Tests for the backend gate.
 *
 *  Coverage:
 *  - Gate open for native JACK
 *  - Gate open for PipeWire-JACK
 *  - Gate closed for ALSA-only
 *  - Gate error for probe error
 *  - Gate closed for unknown capability
 *  - Explanation text for each state
 *  - is_open convenience method
 */

#include <iostream>
#include <string>
#include <cassert>

#include "audio/sooperlooper_backend_gate.hpp"

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
/*  Helpers                                                           */
/* ------------------------------------------------------------------ */

static seq66::backend_probe_result
make_probe (seq66::backend_capability cap, const std::string & impl = "",
            const std::string & err = "")
{
    seq66::backend_probe_result p;
    p.capability = cap;
    p.implementation = impl;
    p.error = err;
    p.server_reachable = (cap == seq66::backend_capability::usable_native_jack ||
                          cap == seq66::backend_capability::usable_pipewire_jack);
    return p;
}

/* ------------------------------------------------------------------ */
/*  Tests                                                             */
/* ------------------------------------------------------------------ */

static void
test_gate_open_native_jack ()
{
    std::cout << "\n--- Gate open native JACK ---" << std::endl;
    seq66::sooperlooper_backend_gate gate;
    auto probe = make_probe(seq66::backend_capability::usable_native_jack,
                            "native");
    auto result = gate.evaluate(probe);
    CHECK(result.is_open());
    CHECK(! result.is_closed());
    CHECK(! result.has_error());
    CHECK(result.explanation.find("native") != std::string::npos);
    CHECK(gate.is_open(probe));
}

static void
test_gate_open_pipewire ()
{
    std::cout << "\n--- Gate open PipeWire-JACK ---" << std::endl;
    seq66::sooperlooper_backend_gate gate;
    auto probe = make_probe(seq66::backend_capability::usable_pipewire_jack,
                            "pipewire");
    auto result = gate.evaluate(probe);
    CHECK(result.is_open());
    CHECK(result.explanation.find("pipewire") != std::string::npos);
    CHECK(gate.is_open(probe));
}

static void
test_gate_closed_alsa ()
{
    std::cout << "\n--- Gate closed ALSA-only ---" << std::endl;
    seq66::sooperlooper_backend_gate gate;
    auto probe = make_probe(seq66::backend_capability::backend_unavailable,
                            "", "no JACK library");
    auto result = gate.evaluate(probe);
    CHECK(! result.is_open());
    CHECK(result.is_closed());
    CHECK(! result.has_error());
    CHECK(result.explanation.find("unavailable") != std::string::npos);
    CHECK(result.explanation.find("JACK") != std::string::npos);
    CHECK(! gate.is_open(probe));
}

static void
test_gate_error ()
{
    std::cout << "\n--- Gate error ---" << std::endl;
    seq66::sooperlooper_backend_gate gate;
    auto probe = make_probe(seq66::backend_capability::probe_error,
                            "", "dlopen failed");
    auto result = gate.evaluate(probe);
    CHECK(! result.is_open());
    CHECK(! result.is_closed());
    CHECK(result.has_error());
    CHECK(result.explanation.find("error") != std::string::npos);
    CHECK(! gate.is_open(probe));
}

static void
test_gate_closed_unknown ()
{
    std::cout << "\n--- Gate closed unknown ---" << std::endl;
    seq66::sooperlooper_backend_gate gate;
    auto probe = make_probe(seq66::backend_capability::unknown);
    auto result = gate.evaluate(probe);
    CHECK(! result.is_open());
    CHECK(result.is_closed());
    CHECK(result.explanation.find("unknown") != std::string::npos);
}

static void
test_explanation ()
{
    std::cout << "\n--- Explanation ---" << std::endl;
    seq66::sooperlooper_backend_gate gate;

    auto probe_jack = make_probe(seq66::backend_capability::usable_native_jack,
                                 "native");
    CHECK(gate.explanation(probe_jack).find("native") != std::string::npos);

    auto probe_alsa = make_probe(seq66::backend_capability::backend_unavailable,
                                 "", "no JACK");
    CHECK(gate.explanation(probe_alsa).find("unavailable") != std::string::npos);
}

static void
test_gate_result_fields ()
{
    std::cout << "\n--- Gate result fields ---" << std::endl;
    seq66::sooperlooper_backend_gate gate;
    auto probe = make_probe(seq66::backend_capability::usable_native_jack,
                            "native");
    auto result = gate.evaluate(probe);
    CHECK_EQ(result.capability, seq66::backend_capability::usable_native_jack);
    CHECK(! result.explanation.empty());
}

/* ------------------------------------------------------------------ */
/*  Main                                                              */
/* ------------------------------------------------------------------ */

int main ()
{
    test_gate_open_native_jack();
    test_gate_open_pipewire();
    test_gate_closed_alsa();
    test_gate_error();
    test_gate_closed_unknown();
    test_explanation();
    test_gate_result_fields();

    std::cout << "\n" << g_total << "/" << g_total
              << " pass. All backend gate assertions passed." << std::endl;
    return g_failed;
}
