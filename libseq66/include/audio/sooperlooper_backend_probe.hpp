#if ! defined SEQ66_SOOPERLOOPER_BACKEND_PROBE_HPP
#define SEQ66_SOOPERLOOPER_BACKEND_PROBE_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_backend_probe.hpp
 *
 *  Provides a side-effect-free typed probe that classifies the audio backend
 *  capability of the current environment without launching any processes.
 *
 *  The probe is injectable: callers provide a detection function so that
 *  tests can supply fake implementations without requiring real JACK libraries.
 */

#include <functional>
#include <string>

namespace seq66
{

/**
 *  Typed classification of audio backend capability.
 */

enum class backend_capability : int
{
    unknown             = -1,
    usable_native_jack  = 0,
    usable_pipewire_jack = 1,
    backend_unavailable = 2,
    probe_error         = 3
};

/**
 *  Evidence returned by the probe.  All fields are informational and
 *  must not influence control flow except through the capability field.
 */

struct backend_probe_result
{
    backend_capability capability {backend_capability::unknown};
    std::string client_library;        /**< "jack" or "pipewire-jack" */
    std::string server_info;           /**< server name/version if available */
    std::string implementation;        /**< "native", "pipewire", or "unknown" */
    std::string error;                 /**< non-empty on probe_error */
    bool server_reachable {false};     /**< true if a JACK server was detected */

    bool is_usable () const
    {
        return capability == backend_capability::usable_native_jack ||
               capability == backend_capability::usable_pipewire_jack;
    }

    bool is_unavailable () const
    {
        return capability == backend_capability::backend_unavailable;
    }

    bool has_error () const
    {
        return capability == backend_capability::probe_error;
    }
};

/**
 *  A detection function that attempts to determine JACK server availability
 *  without starting any processes.  Returns a probe_result with capability
 *  and evidence fields filled in.
 *
 *  The default implementation checks:
 *  1. Whether the JACK client library can be loaded (dlopen).
 *  2. Whether a JACK server is reachable (jack_client_open with NoStart).
 *  3. Whether PipeWire provides the JACK implementation.
 *  4. If none of the above, returns backend_unavailable.
 */

using backend_detect_fn = std::function<backend_probe_result()>;

/**
 *  Run the backend capability probe using the provided detection function.
 *  If no function is provided, uses the default system probe.
 */

backend_probe_result probe_backend_capability (backend_detect_fn detect = nullptr);

/**
 *  Return a human-readable string for a backend_capability value.
 */

const char * to_string (backend_capability cap);

/**
 *  Return a human-readable string for a backend_probe_result.
 */

std::string to_string (const backend_probe_result & result);

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_BACKEND_PROBE_HPP
