#if ! defined SEQ66_SOOPERLOOPER_OSC_CLIENT_HPP
#define SEQ66_SOOPERLOOPER_OSC_CLIENT_HPP

/*
 *  This file is part of seq66 Loves SooperLooper.
 *
 *  seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_osc_client.hpp
 *
 *  OSC client for communicating with SooperLooper.
 */

#include "audio/sooperlooper_recording_types.hpp"

#include <lo/lo.h>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace seq66
{

using osc_message_handler = std::function<void(
    const std::string & path,
    const std::vector<int> & ints,
    const std::vector<float> & floats,
    const std::vector<std::string> & strings)>;

/**
 *  SooperLooper OSC client — non-owning, non-copyable, movable.
 */
class sooperlooper_osc_client
{
public:
    sooperlooper_osc_client (
        const std::string & target_address = "127.0.0.1",
        int target_port = 9951,
        int listen_port = 8000);

    ~sooperlooper_osc_client ();

    sooperlooper_osc_client (const sooperlooper_osc_client &) = delete;
    sooperlooper_osc_client & operator= (const sooperlooper_osc_client &) = delete;
    sooperlooper_osc_client (sooperlooper_osc_client &&) noexcept;
    sooperlooper_osc_client & operator= (sooperlooper_osc_client &&) noexcept;

    bool connect ();
    void disconnect ();
    bool is_connected () const { return m_connected; }

    bool send_message (
        const std::string & path,
        const std::string & types,
        const std::vector<int> & ints = {},
        const std::vector<float> & floats = {},
        const std::vector<std::string> & strings = {});

    bool set_control (int loop_index, const std::string & control, float value);
    bool hit_command (int loop_index, const std::string & command);
    bool request_loop_count ();
    bool request_loop_info (int loop_index);

    void set_message_handler (osc_message_handler handler);
    int poll (int timeout_ms = 0);

    int loop_count () const { return m_loop_count; }

    struct loop_info
    {
        int index{-1};
        int state{0};
        float cycle_size{0};
        float cycle_pos{0};
        float free_time{0};
        float rate{0};
        bool pending{false};
    };

    loop_info get_loop_info (int index) const;

private:
    void * m_target{nullptr};
    void * m_server{nullptr};
    bool m_connected{false};

    int m_loop_count{0};
    std::vector<loop_info> m_loops;

    osc_message_handler m_handler;

    static int osc_handler (
        const char * path,
        const char * types,
        lo_arg ** argv,
        int argc,
        lo_message msg,
        void * user_data);
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_OSC_CLIENT_HPP
