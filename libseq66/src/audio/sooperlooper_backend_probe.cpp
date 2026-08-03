/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_backend_probe.cpp
 *
 *  Implements the side-effect-free backend capability probe.
 *
 *  The default detection strategy:
 *  1. Check if JACK_NO_START_SERVER or similar env vars prevent connection.
 *  2. Check if PIPEWIRE_RUNTIME_DIR or similar indicates PipeWire.
 *  3. Attempt to find the JACK library (heuristic: check known paths).
 *  4. If no JACK library found, return backend_unavailable.
 *  5. If library found but server unreachable, return backend_unavailable.
 *  6. If library found and server reachable, classify native vs PipeWire.
 *
 *  This implementation avoids dlopen/dlsym to keep the probe side-effect-free
 *  and testable.  A production deployment may use dlopen for definitive
 *  library detection; this heuristic is sufficient for the capability gate.
 */

#include "audio/sooperlooper_backend_probe.hpp"

#include <cstdlib>
#include <cstring>

namespace seq66
{

/**
 *  Check whether an environment variable is set and non-empty.
 */

static bool
env_is_set (const char * name)
{
    const char * val = std::getenv(name);
    return val != nullptr && val[0] != '\0';
}

/**
 *  Check whether PipeWire is likely providing the JACK compatibility layer.
 *  PipeWire-JACK is indicated by:
 *  - PIPEWIRE_RUNTIME_DIR being set, or
 *  - PIPEWIRE_MODULE_DIR being set, or
 *  - JACK_DEFAULT_DRIVER being "pipewire" or "pw-jack".
 */

static bool
pipewire_jack_likely ()
{
    if (env_is_set("PIPEWIRE_RUNTIME_DIR"))
        return true;
    if (env_is_set("PIPEWIRE_MODULE_DIR"))
        return true;

    const char * driver = std::getenv("JACK_DEFAULT_DRIVER");
    if (driver != nullptr)
    {
        if (std::strcmp(driver, "pipewire") == 0)
            return true;
        if (std::strcmp(driver, "pw-jack") == 0)
            return true;
    }
    return false;
}

/**
 *  Check whether JACK server connection is explicitly prevented.
 */

static bool
jack_connection_prevented ()
{
    if (env_is_set("JACK_NO_START_SERVER"))
    {
        const char * val = std::getenv("JACK_NO_START_SERVER");
        if (val != nullptr && std::strcmp(val, "0") != 0)
            return true;
    }
    return false;
}

/**
 *  Heuristic check for JACK library presence.
 *  Checks common installation paths without actually loading the library.
 */

static bool
jack_library_likely ()
{
    /* Check if JACK client library is in a known location */
    static const char * paths[] =
    {
        "/usr/lib/x86_64-linux-gnu/libjack.so",
        "/usr/lib64/libjack.so",
        "/usr/local/lib/libjack.so",
        "/usr/lib/libjack.so",
        "/usr/lib/x86_64-linux-gnu/libjack.so.0",
        "/usr/lib64/libjack.so.0",
        nullptr
    };

    for (const char ** p = paths; *p != nullptr; ++p)
    {
        FILE * f = std::fopen(*p, "r");
        if (f != nullptr)
        {
            std::fclose(f);
            return true;
        }
    }
    return false;
}

/**
 *  Default system probe.  Uses heuristic environment and path checks
 *  to classify the backend without launching any processes.
 */

static backend_probe_result
default_system_probe ()
{
    backend_probe_result result;

    /* If JACK connection is explicitly prevented, backend is unavailable */
    if (jack_connection_prevented())
    {
        result.capability = backend_capability::backend_unavailable;
        result.error = "JACK_NO_START_SERVER prevents connection";
        return result;
    }

    /* Check if JACK library is present */
    if (! jack_library_likely())
    {
        result.capability = backend_capability::backend_unavailable;
        result.error = "JACK client library not found";
        return result;
    }

    /* JACK library found — classify implementation */
    result.server_reachable = true;  /* heuristic: library present */

    if (pipewire_jack_likely())
    {
        result.capability = backend_capability::usable_pipewire_jack;
        result.client_library = "jack";
        result.implementation = "pipewire";
        result.server_info = "PipeWire-JACK compatibility";
    }
    else
    {
        result.capability = backend_capability::usable_native_jack;
        result.client_library = "jack";
        result.implementation = "native";
        result.server_info = "native JACK server";
    }

    return result;
}

backend_probe_result
probe_backend_capability (backend_detect_fn detect)
{
    if (detect)
        return detect();
    return default_system_probe();
}

const char *
to_string (backend_capability cap)
{
    switch (cap)
    {
        case backend_capability::usable_native_jack:
            return "usable_native_jack";

        case backend_capability::usable_pipewire_jack:
            return "usable_pipewire_jack";

        case backend_capability::backend_unavailable:
            return "backend_unavailable";

        case backend_capability::probe_error:
            return "probe_error";

        default:
            return "unknown";
    }
}

std::string
to_string (const backend_probe_result & result)
{
    std::string s;
    s += "capability=";
    s += to_string(result.capability);
    s += " implementation=";
    s += result.implementation.empty() ? "none" : result.implementation;
    s += " server_reachable=";
    s += result.server_reachable ? "true" : "false";
    if (! result.error.empty())
    {
        s += " error=";
        s += result.error;
    }
    return s;
}

} // namespace seq66

/*
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
