#if ! defined SEQ66_SOOPERLOOPER_JACK_TRANSPORT_OBSERVER_HPP
#define SEQ66_SOOPERLOOPER_JACK_TRANSPORT_OBSERVER_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_jack_transport_observer.hpp
 *
 *  JACK transport observer — provides real transport state from JACK.
 *
 *  This class implements the transport_state_provider interface
 *  using JACK's transport API.
 */

#include "audio/sooperlooper_recording_scheduler.hpp"

#include <jack/jack.h>
#include <jack/transport.h>

namespace seq66
{

/**
 *  JACK transport observer.
 */
class jack_transport_observer : public transport_state_provider
{
public:
    jack_transport_observer ();
    ~jack_transport_observer () override;

    /** Connect to JACK. */
    bool connect (const std::string & client_name = "seq66-recording");

    /** Disconnect from JACK. */
    void disconnect ();

    /** Is connected? */
    bool is_jack_connected () const { return m_client != nullptr; }

    /** transport_state_provider interface. */
    tick_position current_tick () const override;
    transport_generation current_generation () const override;
    bool is_running () const override;
    musical_metric capture_metric () const override;

private:
    jack_client_t * m_client{nullptr};
    mutable int64_t m_generation{0};
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_JACK_TRANSPORT_OBSERVER_HPP
