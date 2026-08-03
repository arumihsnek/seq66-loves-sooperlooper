/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_tempo_policy.cpp
 *
 *  Tempo policy implementation.
 */

#include "audio/sooperlooper_tempo_policy.hpp"

#include <algorithm>
#include <cmath>

namespace seq66
{

static constexpr float RATE_MIN = 0.25f;
static constexpr float RATE_MAX = 4.0f;

tempo_mode
sooperlooper_tempo_policy::mode () const
{
    return m_mode;
}

void
sooperlooper_tempo_policy::set_mode (tempo_mode m)
{
    m_mode = m;
    recompute();
}

bool
sooperlooper_tempo_policy::set_mode (const std::string & name)
{
    tempo_mode m;
    if (try_parse(name, m))
    {
        set_mode(m);
        return true;
    }
    return false;
}

const global_tempo_config &
sooperlooper_tempo_policy::global_config () const
{
    return m_global;
}

void
sooperlooper_tempo_policy::set_global_tempo (float bpm)
{
    m_global.tempo = std::max(0.0f, std::min(1000.0f, bpm));
    recompute();
}

const std::vector<loop_tempo_config> &
sooperlooper_tempo_policy::loop_configs () const
{
    return m_loops;
}

int
sooperlooper_tempo_policy::add_loop (float recording_tempo)
{
    loop_tempo_config lc;
    lc.recording_tempo = std::max(1.0f, recording_tempo);
    lc.runtime_index = static_cast<int>(m_loops.size());
    m_loops.push_back(lc);
    recompute();
    return lc.runtime_index;
}

bool
sooperlooper_tempo_policy::remove_loop (int index)
{
    if (index < 0 || index >= static_cast<int>(m_loops.size()))
        return false;
    m_loops.erase(m_loops.begin() + index);
    recompute();
    return true;
}

void
sooperlooper_tempo_policy::clear_loops ()
{
    m_loops.clear();
}

void
sooperlooper_tempo_policy::recompute ()
{
    for (auto & lc : m_loops)
    {
        switch (m_mode)
        {
            case tempo_mode::free:
                lc.use_rate = false;
                lc.tempo_stretch = false;
                lc.rate = 1.0f;
                lc.round_integer_tempo = false;
                lc.stretch_ratio = 1.0f;
                break;

            case tempo_mode::tape:
                lc.use_rate = true;
                lc.tempo_stretch = false;
                lc.rate = 1.0f;
                lc.round_integer_tempo = false;
                lc.stretch_ratio = 1.0f;
                break;

            case tempo_mode::elastic:
                lc.use_rate = true;
                lc.tempo_stretch = true;
                lc.rate = compute_rate(m_global.tempo, lc.recording_tempo);
                lc.round_integer_tempo = true;
                lc.stretch_ratio = lc.rate;
                break;
        }
    }
}

const loop_tempo_config *
sooperlooper_tempo_policy::loop_config (int index) const
{
    if (index < 0 || index >= static_cast<int>(m_loops.size()))
        return nullptr;
    return &m_loops[index];
}

float
sooperlooper_tempo_policy::compute_rate (
    float global_tempo,
    float recording_tempo
) const
{
    if (recording_tempo <= 0.0f)
        return 1.0f;
    float r = global_tempo / recording_tempo;
    return std::max(RATE_MIN, std::min(RATE_MAX, r));
}

const char *
to_string (tempo_mode m)
{
    switch (m)
    {
        case tempo_mode::free:    return "free";
        case tempo_mode::tape:    return "tape";
        case tempo_mode::elastic: return "elastic";
    }
    return "unknown";
}

bool
try_parse (const std::string & name, tempo_mode & m)
{
    if (name == "free")    { m = tempo_mode::free;    return true; }
    if (name == "tape")    { m = tempo_mode::tape;    return true; }
    if (name == "elastic") { m = tempo_mode::elastic; return true; }
    return false;
}

} // namespace seq66

/*
 * sooperlooper_tempo_policy.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
