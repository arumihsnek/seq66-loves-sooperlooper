/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_command_confirmation.cpp
 *
 *  Command confirmation tracker implementation.
 */

#include "audio/sooperlooper_command_confirmation.hpp"

#include <algorithm>

namespace seq66
{

void
command_confirmation_tracker::set_reconciler (reconciliation_fn fn)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_reconciler = std::move(fn);
}

bool
command_confirmation_tracker::track (const std::string & uuid,
                                     const std::string & description,
                                     const std::string & osc_path,
                                     int expected_state,
                                     int loop_index,
                                     int deadline_ms)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_operations.count(uuid))
        return false;   // Duplicate UUID.

    pending_operation op;
    op.uuid = uuid;
    op.description = description;
    op.osc_path = osc_path;
    op.expected_state = expected_state;
    op.loop_index = loop_index;
    op.sent_at = std::chrono::steady_clock::now();
    op.deadline = op.sent_at + std::chrono::milliseconds(deadline_ms);
    op.outcome = confirmation_outcome::pending;

    m_operations[uuid] = std::move(op);
    return true;
}

bool
command_confirmation_tracker::confirm (int loop_index, int new_state)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto & [uuid, op] : m_operations)
    {
        if (op.outcome != confirmation_outcome::pending)
            continue;
        if (op.loop_index >= 0 && op.loop_index != loop_index)
            continue;
        if (op.expected_state >= 0 && op.expected_state != new_state)
            continue;

        op.outcome = confirmation_outcome::confirmed;
        return true;
    }
    return false;
}

bool
command_confirmation_tracker::confirm_by_uuid (const std::string & uuid)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_operations.find(uuid);
    if (it == m_operations.end())
        return false;
    if (it->second.outcome != confirmation_outcome::pending)
        return false;
    it->second.outcome = confirmation_outcome::confirmed;
    return true;
}

bool
command_confirmation_tracker::fail (const std::string & uuid,
                                    const std::string & error)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_operations.find(uuid);
    if (it == m_operations.end())
        return false;
    if (it->second.outcome != confirmation_outcome::pending)
        return false;
    it->second.outcome = confirmation_outcome::failed;
    it->second.error = error;
    return true;
}

bool
command_confirmation_tracker::cancel (const std::string & uuid)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_operations.find(uuid);
    if (it == m_operations.end())
        return false;
    if (it->second.outcome != confirmation_outcome::pending)
        return false;
    it->second.outcome = confirmation_outcome::cancelled;
    return true;
}

int
command_confirmation_tracker::evaluate ()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int count = 0;
    auto now = std::chrono::steady_clock::now();

    for (auto & [uuid, op] : m_operations)
    {
        if (op.outcome != confirmation_outcome::pending)
            continue;
        if (now >= op.deadline)
        {
            op.outcome = confirmation_outcome::indeterminate;
            ++count;
        }
    }
    return count;
}

int
command_confirmation_tracker::reconcile ()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int count = 0;

    if (!m_reconciler)
        return 0;

    for (auto & [uuid, op] : m_operations)
    {
        if (op.outcome != confirmation_outcome::indeterminate)
            continue;
        if (op.loop_index < 0)
            continue;

        if (m_reconciler(op.loop_index))
        {
            op.outcome = confirmation_outcome::confirmed;
            ++count;
        }
    }
    return count;
}

confirmation_outcome
command_confirmation_tracker::outcome (const std::string & uuid) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_operations.find(uuid);
    if (it == m_operations.end())
        return confirmation_outcome::cancelled;
    return it->second.outcome;
}

std::vector<pending_operation>
command_confirmation_tracker::pending () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<pending_operation> result;
    for (const auto & [uuid, op] : m_operations)
    {
        if (op.outcome == confirmation_outcome::pending)
            result.push_back(op);
    }
    return result;
}

std::vector<pending_operation>
command_confirmation_tracker::by_outcome (confirmation_outcome o) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<pending_operation> result;
    for (const auto & [uuid, op] : m_operations)
    {
        if (op.outcome == o)
            result.push_back(op);
    }
    return result;
}

std::size_t
command_confirmation_tracker::pending_count () const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::size_t count = 0;
    for (const auto & [uuid, op] : m_operations)
    {
        if (op.outcome == confirmation_outcome::pending)
            ++count;
    }
    return count;
}

void
command_confirmation_tracker::clear ()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_operations.clear();
}

} // namespace seq66

/*
 * sooperlooper_command_confirmation.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
