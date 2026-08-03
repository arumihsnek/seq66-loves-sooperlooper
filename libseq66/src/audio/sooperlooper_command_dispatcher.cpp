/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_command_dispatcher.cpp
 *
 *  Performer audio command dispatch implementation.
 */

#include "audio/sooperlooper_command_dispatcher.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <sstream>

namespace seq66
{

sooperlooper_command_dispatcher::sooperlooper_command_dispatcher ()
{
}

sooperlooper_command_dispatcher::~sooperlooper_command_dispatcher ()
{
}

void
sooperlooper_command_dispatcher::set_submit_callback (command_submit_fn fn)
{
    m_submit_fn = fn;
}

void
sooperlooper_command_dispatcher::set_status_callback (
    command_status_change_fn fn
)
{
    m_status_fn = fn;
}

void
sooperlooper_command_dispatcher::set_clip_mapper (
    sooperlooper_clip_mapper * mapper
)
{
    m_mapper = mapper;
}

void
sooperlooper_command_dispatcher::set_confirmation_tracker (
    command_confirmation_tracker * tracker
)
{
    m_tracker = tracker;
}

void
sooperlooper_command_dispatcher::set_observed_cache (
    sooperlooper_observed_cache * cache
)
{
    m_cache = cache;
}

std::string
sooperlooper_command_dispatcher::dispatch (
    const std::string & clip_uuid,
    sooperlooper_command cmd,
    const std::string & description,
    int deadline_ms
)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (clip_uuid.empty())
        return std::string();

    /* Resolve clip UUID to runtime index. */
    int runtime_index = -1;
    if (m_mapper)
        runtime_index = m_mapper->lookup(clip_uuid);

    std::string uuid = generate_uuid();
    audio_command ac;
    ac.uuid = uuid;
    ac.clip_uuid = clip_uuid;
    ac.command = cmd;
    ac.runtime_index = runtime_index;
    ac.generation = m_generation;
    ac.status = command_status::desired;
    ac.requested_at = std::chrono::steady_clock::now();
    ac.description = description;
    ac.deadline = ac.requested_at + std::chrono::milliseconds(deadline_ms);

    m_commands[uuid] = ac;
    return uuid;
}

int
sooperlooper_command_dispatcher::submit_pending ()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int submitted = 0;

    for (auto & [uuid, cmd] : m_commands)
    {
        if (cmd.status != command_status::desired)
            continue;

        /* Resolve clip UUID if not yet resolved. */
        if (cmd.runtime_index < 0 && m_mapper)
            cmd.runtime_index = m_mapper->lookup(cmd.clip_uuid);

        /* Skip if still unresolved. */
        if (cmd.runtime_index < 0)
            continue;

        /* Skip if no submit callback. */
        if (! m_submit_fn)
            continue;

        /* Call the submit callback. */
        bool ok = m_submit_fn(cmd.runtime_index, cmd.command);
        if (! ok)
        {
            transition(cmd, command_status::failed, "submit callback failed");
            continue;
        }

        /* Register with confirmation tracker. */
        if (m_tracker)
        {
            auto deadline_ms = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    cmd.deadline - std::chrono::steady_clock::now()
                ).count()
            );
            if (deadline_ms < 100)
                deadline_ms = 100;

            m_tracker->track(
                cmd.uuid,
                cmd.description,
                "/sl/" + std::to_string(cmd.runtime_index) + "/hit",
                -1, /* expected_state: any */
                cmd.runtime_index,
                cmd.generation,
                "", /* expected_control */
                deadline_ms
            );
        }

        cmd.submitted_at = std::chrono::steady_clock::now();
        transition(cmd, command_status::pending);
        ++submitted;
    }

    return submitted;
}

int
sooperlooper_command_dispatcher::check_feedback ()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int resolved = 0;

    auto now = std::chrono::steady_clock::now();

    for (auto & [uuid, cmd] : m_commands)
    {
        if (cmd.status != command_status::pending)
            continue;

        /* Check deadline. */
        if (now > cmd.deadline)
        {
            /* Try reconciliation via tracker if available. */
            bool reconciled = false;
            if (m_tracker)
            {
                auto outcome = m_tracker->outcome(uuid);
                if (outcome == confirmation_outcome::confirmed)
                {
                    reconciled = true;
                }
            }

            if (reconciled)
            {
                transition(cmd, command_status::confirmed);
            }
            else
            {
                transition(
                    cmd, command_status::indeterminate,
                    "deadline expired"
                );
            }
            ++resolved;
            continue;
        }

        /* Query observed state cache. */
        if (m_cache && cmd.runtime_index >= 0)
        {
            auto snap = m_cache->snapshot();
            auto it = snap.loops.find(cmd.runtime_index);
            if (it != snap.loops.end())
            {
                /* If the loop state changed to something that looks
                 * like a confirmation, confirm the command. */
                const auto & loop = it->second;
                if (loop.state.present)
                {
                    /* For now: any state feedback after submission
                     * is treated as implicit confirmation for simple
                     * commands.  The confirmation tracker handles
                     * precise state-transition matching. */
                    if (m_tracker)
                    {
                        auto outcome = m_tracker->outcome(uuid);
                        if (outcome == confirmation_outcome::confirmed)
                        {
                            transition(cmd, command_status::confirmed);
                            ++resolved;
                        }
                    }
                }
            }
        }
    }

    return resolved;
}

bool
sooperlooper_command_dispatcher::cancel (const std::string & uuid)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_commands.find(uuid);
    if (it == m_commands.end())
        return false;

    auto & cmd = it->second;
    if (cmd.status != command_status::desired &&
        cmd.status != command_status::pending)
    {
        return false;
    }

    if (m_tracker)
        m_tracker->cancel(uuid);

    transition(cmd, command_status::cancelled);
    return true;
}

int
sooperlooper_command_dispatcher::cancel_generation (
    std::uint64_t generation
)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int cancelled = 0;

    for (auto & [uuid, cmd] : m_commands)
    {
        if (cmd.generation == generation &&
            (cmd.status == command_status::desired ||
             cmd.status == command_status::pending))
        {
            if (m_tracker)
                m_tracker->cancel(uuid);

            transition(cmd, command_status::cancelled);
            ++cancelled;
        }
    }

    return cancelled;
}

audio_command
sooperlooper_command_dispatcher::command_by_uuid (
    const std::string & uuid
) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_commands.find(uuid);
    if (it != m_commands.end())
        return it->second;

    return audio_command();
}

std::vector<audio_command>
sooperlooper_command_dispatcher::commands_by_status (
    command_status status
) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<audio_command> result;

    for (const auto & [uuid, cmd] : m_commands)
    {
        if (cmd.status == status)
            result.push_back(cmd);
    }

    return result;
}

std::vector<audio_command>
sooperlooper_command_dispatcher::all_commands () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<audio_command> result;

    for (const auto & [uuid, cmd] : m_commands)
        result.push_back(cmd);

    return result;
}

int
sooperlooper_command_dispatcher::count_by_status (
    command_status status
) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int count = 0;

    for (const auto & [uuid, cmd] : m_commands)
    {
        if (cmd.status == status)
            ++count;
    }

    return count;
}

std::uint64_t
sooperlooper_command_dispatcher::generation () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_generation;
}

void
sooperlooper_command_dispatcher::set_generation (std::uint64_t gen)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_generation = gen;
}

void
sooperlooper_command_dispatcher::clear ()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_commands.clear();
}

void
sooperlooper_command_dispatcher::transition (
    audio_command & cmd,
    command_status new_status,
    const std::string & error
)
{
    cmd.status = new_status;
    if (! error.empty())
        cmd.error = error;

    if (m_status_fn)
        m_status_fn(cmd);
}

std::string
sooperlooper_command_dispatcher::generate_uuid () const
{
    static std::atomic<unsigned long> counter{0};
    unsigned long id = counter.fetch_add(1, std::memory_order_relaxed);

    std::ostringstream oss;
    oss << "cmd-" << m_generation << "-" << id;
    return oss.str();
}

} // namespace seq66

/*
 * sooperlooper_command_dispatcher.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
