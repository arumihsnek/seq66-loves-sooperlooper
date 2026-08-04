/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_jack_transport_observer.cpp
 *
 *  JACK transport observer implementation.
 */

#include "audio/sooperlooper_jack_transport_observer.hpp"

namespace seq66
{

jack_transport_observer::jack_transport_observer ()
{
}

jack_transport_observer::~jack_transport_observer ()
{
    disconnect();
}

bool
jack_transport_observer::connect (const std::string & client_name)
{
    jack_status_t status;
    m_client = jack_client_open(client_name.c_str(), JackNoStartServer, &status);
    return m_client != nullptr;
}

void
jack_transport_observer::disconnect ()
{
    if (m_client)
    {
        jack_client_close(m_client);
        m_client = nullptr;
    }
}

tick_position
jack_transport_observer::current_tick () const
{
    if (! m_client)
        return tick_position(0);

    jack_nframes_t frame = jack_get_current_transport_frame(m_client);
    return tick_position(static_cast<int64_t>(frame));
}

transport_generation
jack_transport_observer::current_generation () const
{
    if (! m_client)
        return transport_generation(0);

    return transport_generation(m_generation++);
}

bool
jack_transport_observer::is_running () const
{
    if (! m_client)
        return false;

    jack_position_t pos;
    jack_transport_state_t state = jack_transport_query(m_client, &pos);
    return state == JackTransportRolling;
}

musical_metric
jack_transport_observer::capture_metric () const
{
    musical_metric m;

    if (! m_client)
        return m;

    jack_position_t pos;
    jack_transport_query(m_client, &pos);

    if (pos.valid & JackPositionBBT)
    {
        m.ticks_per_quarter = pos.ticks_per_beat;
        m.numerator = pos.beats_per_bar;
        m.denominator = 4;
        m.ticks_per_beat = pos.ticks_per_beat;
        m.beats_per_bar = pos.beats_per_bar;
        m.start_tempo_bpm = pos.beats_per_minute;
    }

    m.generation = current_generation();
    return m;
}

} // namespace seq66
