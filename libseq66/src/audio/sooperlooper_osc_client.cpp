/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_osc_client.cpp
 *
 *  SooperLooper OSC client implementation.
 */

#include "audio/sooperlooper_osc_client.hpp"

#include <lo/lo.h>
#include <cstring>

namespace seq66
{

// ======================================================================
//  Construction / destruction
// ======================================================================

sooperlooper_osc_client::sooperlooper_osc_client (
    const std::string & target_address,
    int target_port,
    int listen_port)
{
    m_target = lo_address_new(target_address.c_str(),
        std::to_string(target_port).c_str());

    m_server = lo_server_new(
        std::to_string(listen_port).c_str(),
        nullptr);
}

sooperlooper_osc_client::~sooperlooper_osc_client ()
{
    disconnect();
    if (m_target)
        lo_address_free(m_target);
    if (m_server)
        lo_server_free(m_server);
}

sooperlooper_osc_client::sooperlooper_osc_client (
    sooperlooper_osc_client && other) noexcept
    : m_target(other.m_target)
    , m_server(other.m_server)
    , m_connected(other.m_connected)
    , m_loop_count(other.m_loop_count)
    , m_loops(std::move(other.m_loops))
    , m_handler(std::move(other.m_handler))
{
    other.m_target = nullptr;
    other.m_server = nullptr;
    other.m_connected = false;
}

sooperlooper_osc_client &
sooperlooper_osc_client::operator= (sooperlooper_osc_client && other) noexcept
{
    if (this != &other)
    {
        disconnect();
        if (m_target) lo_address_free(m_target);
        if (m_server) lo_server_free(m_server);

        m_target = other.m_target;
        m_server = other.m_server;
        m_connected = other.m_connected;
        m_loop_count = other.m_loop_count;
        m_loops = std::move(other.m_loops);
        m_handler = std::move(other.m_handler);

        other.m_target = nullptr;
        other.m_server = nullptr;
        other.m_connected = false;
    }
    return *this;
}

// ======================================================================
//  Connection
// ======================================================================

bool
sooperlooper_osc_client::connect ()
{
    if (! m_target || ! m_server)
        return false;

    /* Register handler for incoming messages. */
    lo_server_add_method(m_server, nullptr, nullptr, osc_handler, this);

    m_connected = true;
    return true;
}

void
sooperlooper_osc_client::disconnect ()
{
    m_connected = false;
}

// ======================================================================
//  Commands
// ======================================================================

bool
sooperlooper_osc_client::send_message (
    const std::string & path,
    const std::string & types,
    const std::vector<int> & ints,
    const std::vector<float> & floats,
    const std::vector<std::string> & strings)
{
    if (! m_connected || ! m_target)
        return false;

    lo_message msg = lo_message_new();

    /* Build argument list based on type tags. */
    size_t int_idx = 0;
    size_t float_idx = 0;
    size_t str_idx = 0;

    for (const char * t = types.c_str(); *t; ++t)
    {
        switch (*t)
        {
            case 'i':
                if (int_idx < ints.size())
                    lo_message_add_int32(msg, ints[int_idx++]);
                break;

            case 'f':
                if (float_idx < floats.size())
                    lo_message_add_float(msg, floats[float_idx++]);
                break;

            case 's':
                if (str_idx < strings.size())
                    lo_message_add_string(msg, strings[str_idx++].c_str());
                break;

            case 'N':
                lo_message_add_nil(msg);
                break;

            default:
                break;
        }
    }

    int result = lo_send_message(m_target, path.c_str(), msg);
    lo_message_free(msg);

    return result >= 0;
}

bool
sooperlooper_osc_client::set_control (
    int loop_index,
    const std::string & control,
    float value)
{
    return send_message("/set", "isf",
        {loop_index}, {value}, {control});
}

bool
sooperlooper_osc_client::hit_command (int loop_index, const std::string & command)
{
    return send_message("/hit", "is",
        {loop_index}, {}, {command});
}

bool
sooperlooper_osc_client::request_loop_count ()
{
    return send_message("/loop_count", "", {}, {}, {});
}

bool
sooperlooper_osc_client::request_loop_info (int loop_index)
{
    return send_message("/loop_info", "i",
        {loop_index}, {}, {});
}

// ======================================================================
//  Reception
// ======================================================================

void
sooperlooper_osc_client::set_message_handler (osc_message_handler handler)
{
    m_handler = std::move(handler);
}

int
sooperlooper_osc_client::poll (int timeout_ms)
{
    if (! m_server)
        return 0;

    return lo_server_recv_noblock(m_server, timeout_ms);
}

int
sooperlooper_osc_client::osc_handler (
    const char * path,
    const char * types,
    lo_arg ** argv,
    int argc,
    lo_message /* msg */,
    void * user_data)
{
    auto * self = static_cast<sooperlooper_osc_client *>(user_data);

    std::vector<int> ints;
    std::vector<float> floats;
    std::vector<std::string> strings;

    for (int i = 0; i < argc; ++i)
    {
        switch (types[i])
        {
            case 'i':
                ints.push_back(argv[i]->i);
                break;
            case 'f':
                floats.push_back(argv[i]->f);
                break;
            case 's':
                strings.push_back(&argv[i]->s);
                break;
            default:
                break;
        }
    }

    /* Handle known responses. */
    if (std::string(path) == "/loop_count" && ! ints.empty())
    {
        self->m_loop_count = ints[0];
        self->m_loops.resize(ints[0]);
    }

    /* Forward to user handler. */
    if (self->m_handler)
        self->m_handler(path, ints, floats, strings);

    return 0;
}

// ======================================================================
//  Loop state
// ======================================================================

sooperlooper_osc_client::loop_info
sooperlooper_osc_client::get_loop_info (int index) const
{
    if (index >= 0 && index < static_cast<int>(m_loops.size()))
        return m_loops[index];
    return loop_info{};
}

} // namespace seq66

/*
 * sooperlooper_osc_client.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
