/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */

/**
 * \file          sooperlooper_process_supervisor.cpp
 *
 *  Implements the managed process supervisor for a headless SooperLooper
 *  engine.
 *
 *  The supervisor owns one child process at a time, verifies its identity
 *  by PID, and performs bounded shutdown escalation.  All process operations
 *  are delegated to an injectable process_adapter for testability.
 */

#include "audio/sooperlooper_process_supervisor.hpp"

#include <cassert>

namespace seq66
{

/* ------------------------------------------------------------------ */
/*  Construction and configuration                                     */
/* ------------------------------------------------------------------ */

sooperlooper_process_supervisor::sooperlooper_process_supervisor ()
    : m_adapter(nullptr)
{
}

sooperlooper_process_supervisor::sooperlooper_process_supervisor
(
    std::shared_ptr<process_adapter> adapter
)
    : m_adapter(std::move(adapter))
{
}

sooperlooper_process_supervisor::~sooperlooper_process_supervisor ()
{
    if (m_state == supervisor_state::running ||
        m_state == supervisor_state::starting)
    {
        shutdown();
    }
}

void
sooperlooper_process_supervisor::set_config (const supervisor_config & cfg)
{
    if (m_state == supervisor_state::idle ||
        m_state == supervisor_state::failed)
    {
        m_config = cfg;
    }
}

const supervisor_config &
sooperlooper_process_supervisor::get_config () const
{
    return m_config;
}

/* ------------------------------------------------------------------ */
/*  Launch                                                             */
/* ------------------------------------------------------------------ */

launch_result
sooperlooper_process_supervisor::launch ()
{
    if (m_state == supervisor_state::running ||
        m_state == supervisor_state::starting)
    {
        m_last_error = "child already running";
        return launch_result::already_running;
    }

    if (! m_adapter)
    {
        m_state = supervisor_state::failed;
        m_last_error = "no process adapter configured";
        return launch_result::fork_failed;
    }

    if (m_config.executable_path.empty())
    {
        m_state = supervisor_state::failed;
        m_last_error = "executable path not configured";
        return launch_result::exec_failed;
    }

    ++m_generation;
    m_state = supervisor_state::starting;
    m_last_error.clear();

    std::vector<std::string> argv = build_argv();
    int child_pid = 0;

    if (! m_adapter->spawn(argv, child_pid))
    {
        m_state = supervisor_state::failed;
        m_last_error = "fork/exec failed";
        return launch_result::fork_failed;
    }

    if (! verify_identity(child_pid))
    {
        m_state = supervisor_state::failed;
        m_last_error = "identity verification failed for PID " +
                       std::to_string(child_pid);
        return launch_result::identity_mismatch;
    }

    m_child_pid = child_pid;
    m_expected_pid = child_pid;
    m_state = supervisor_state::running;
    return launch_result::success;
}

/* ------------------------------------------------------------------ */
/*  Shutdown                                                           */
/* ------------------------------------------------------------------ */

bool
sooperlooper_process_supervisor::shutdown ()
{
    if (m_state == supervisor_state::idle)
        return true;

    if (! m_adapter)
    {
        m_state = supervisor_state::idle;
        m_child_pid = 0;
        return true;
    }

    bool stopped = false;

    if (m_state == supervisor_state::running && m_child_pid > 0)
    {
        /* Phase 1: Graceful (OSC /quit) */
        stopped = graceful_shutdown();
        if (stopped)
        {
            m_state = supervisor_state::idle;
            m_child_pid = 0;
            return true;
        }

        /* Phase 2: SIGTERM */
        stopped = term_shutdown();
        if (stopped)
        {
            m_state = supervisor_state::idle;
            m_child_pid = 0;
            return true;
        }

        /* Phase 3: SIGKILL */
        stopped = kill_shutdown();
        m_state = stopped ? supervisor_state::idle : supervisor_state::failed;
        m_child_pid = 0;
        return stopped;
    }

    /* If starting or stopping, just kill directly */
    if (m_child_pid > 0)
    {
        m_adapter->send_signal(m_child_pid, 9);  /* SIGKILL */
    }

    m_state = supervisor_state::idle;
    m_child_pid = 0;
    return true;
}

void
sooperlooper_process_supervisor::force_kill ()
{
    if (m_child_pid > 0 && m_adapter)
    {
        m_adapter->send_signal(m_child_pid, 9);  /* SIGKILL */
    }
    m_state = supervisor_state::idle;
    m_child_pid = 0;
}

launch_result
sooperlooper_process_supervisor::restart ()
{
    if (m_config.max_restarts > 0 &&
        m_restart_count >= m_config.max_restarts)
    {
        m_last_error = "max restart limit reached";
        return launch_result::fork_failed;
    }

    if (m_state == supervisor_state::running ||
        m_state == supervisor_state::starting)
    {
        shutdown();
    }

    ++m_restart_count;
    m_state = supervisor_state::cooldown;
    return launch();
}

/* ------------------------------------------------------------------ */
/*  State queries                                                      */
/* ------------------------------------------------------------------ */

supervisor_state
sooperlooper_process_supervisor::state () const
{
    return m_state;
}

std::uint64_t
sooperlooper_process_supervisor::generation () const
{
    return m_generation;
}

int
sooperlooper_process_supervisor::child_pid () const
{
    return m_child_pid;
}

bool
sooperlooper_process_supervisor::child_alive () const
{
    if (m_child_pid <= 0 || ! m_adapter)
        return false;
    return m_adapter->is_alive(m_child_pid);
}

const std::string &
sooperlooper_process_supervisor::last_error () const
{
    return m_last_error;
}

int
sooperlooper_process_supervisor::restart_count () const
{
    return m_restart_count;
}

/* ------------------------------------------------------------------ */
/*  Polling                                                            */
/* ------------------------------------------------------------------ */

bool
sooperlooper_process_supervisor::poll ()
{
    if (m_state != supervisor_state::running)
        return false;

    if (m_child_pid > 0 && m_adapter && ! m_adapter->is_alive(m_child_pid))
    {
        m_state = supervisor_state::failed;
        m_last_error = "child process exited unexpectedly (PID " +
                       std::to_string(m_child_pid) + ")";
        m_child_pid = 0;
        return true;
    }

    return false;
}

void
sooperlooper_process_supervisor::force_state (supervisor_state s)
{
    m_state = s;
}

/* ------------------------------------------------------------------ */
/*  Argument vector                                                    */
/* ------------------------------------------------------------------ */

std::vector<std::string>
sooperlooper_process_supervisor::build_argv () const
{
    std::vector<std::string> argv;
    argv.push_back(m_config.executable_path);
    for (const auto & arg : m_config.args)
        argv.push_back(arg);
    return argv;
}

/* ------------------------------------------------------------------ */
/*  Private helpers                                                    */
/* ------------------------------------------------------------------ */

bool
sooperlooper_process_supervisor::verify_identity (int pid)
{
    if (pid <= 0)
        return false;

    if (! m_adapter)
        return false;

    /* Verify the process is alive immediately after spawn */
    return m_adapter->is_alive(pid);
}

bool
sooperlooper_process_supervisor::graceful_shutdown ()
{
    /*
     * Graceful shutdown sends an OSC /quit to the engine.
     * In the real implementation this would use the client.
     * Here we wait for the process to exit within the timeout.
     * If it does, great.  If not, the caller escalates.
     */
    if (m_child_pid <= 0 || ! m_adapter)
        return true;

    return m_adapter->wait_exit(m_child_pid, m_config.graceful_timeout_ms);
}

bool
sooperlooper_process_supervisor::term_shutdown ()
{
    if (m_child_pid <= 0 || ! m_adapter)
        return true;

    m_adapter->send_signal(m_child_pid, 15);  /* SIGTERM */
    return m_adapter->wait_exit(m_child_pid, m_config.term_timeout_ms);
}

bool
sooperlooper_process_supervisor::kill_shutdown ()
{
    if (m_child_pid <= 0 || ! m_adapter)
        return true;

    m_adapter->send_signal(m_child_pid, 9);  /* SIGKILL */
    /* SIGKILL should be near-instant, wait briefly */
    return m_adapter->wait_exit(m_child_pid, 1000);
}

} // namespace seq66

/*
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
