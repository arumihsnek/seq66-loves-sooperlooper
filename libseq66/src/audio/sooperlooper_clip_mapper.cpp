/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_clip_mapper.cpp
 *
 *  Stable clip UUID to runtime-index mapper implementation.
 */

#include "audio/sooperlooper_clip_mapper.hpp"

#include <algorithm>
#include <cassert>

namespace seq66
{

sooperlooper_clip_mapper::sooperlooper_clip_mapper ()
{
}

sooperlooper_clip_mapper::~sooperlooper_clip_mapper ()
{
}

void
sooperlooper_clip_mapper::set_allocation_policy (allocation_policy policy)
{
    m_policy = policy;
}

allocation_policy
sooperlooper_clip_mapper::get_allocation_policy () const
{
    return m_policy;
}

int
sooperlooper_clip_mapper::register_clip (const std::string & uuid)
{
    if (uuid.empty())
        return -1;

    /* Reject duplicate UUIDs. */
    if (m_uuid_to_index.count(uuid) > 0)
        return -1;

    int idx = next_index();
    if (idx < 0)
        return -1;

    clip_entry entry;
    entry.uuid = uuid;
    entry.runtime_index = idx;
    entry.generation = m_generation;
    entry.active = true;

    m_entries.push_back(entry);
    m_uuid_to_index[uuid] = static_cast<int>(m_entries.size() - 1);
    m_index_to_uuid[idx] = uuid;

    return idx;
}

bool
sooperlooper_clip_mapper::unregister_clip (const std::string & uuid)
{
    auto it = m_uuid_to_index.find(uuid);
    if (it == m_uuid_to_index.end())
        return false;

    int vec_index = it->second;
    if (vec_index >= 0 && vec_index < static_cast<int>(m_entries.size()))
    {
        int runtime_idx = m_entries[vec_index].runtime_index;
        m_index_to_uuid.erase(runtime_idx);
        m_entries.erase(m_entries.begin() + vec_index);
    }

    m_uuid_to_index.erase(it);

    /* Rebuild vector-to-index mapping after erase. */
    rebuild_index_map();
    return true;
}

int
sooperlooper_clip_mapper::lookup (const std::string & uuid) const
{
    auto it = m_uuid_to_index.find(uuid);
    if (it == m_uuid_to_index.end())
        return -1;

    int vec_index = it->second;
    if (vec_index < 0 || vec_index >= static_cast<int>(m_entries.size()))
        return -1;

    const clip_entry & e = m_entries[vec_index];
    if (! e.active || e.generation != m_generation)
        return -1;

    return e.runtime_index;
}

std::string
sooperlooper_clip_mapper::reverse_lookup (int runtime_index) const
{
    auto it = m_index_to_uuid.find(runtime_index);
    if (it == m_index_to_uuid.end())
        return std::string();

    return it->second;
}

void
sooperlooper_clip_mapper::invalidate_generation (std::uint64_t generation)
{
    for (auto & e : m_entries)
    {
        if (e.generation == generation)
        {
            e.active = false;
            e.runtime_index = -1;
        }
    }
    m_index_to_uuid.clear();

    if (m_on_invalidate)
        m_on_invalidate(generation);
}

int
sooperlooper_clip_mapper::advance_generation (std::uint64_t new_generation)
{
    m_generation = new_generation;
    int remapped = 0;

    for (auto & e : m_entries)
    {
        e.generation = new_generation;
        e.runtime_index = next_index();
        e.active = true;
        m_index_to_uuid[e.runtime_index] = e.uuid;
        ++remapped;
    }

    rebuild_index_map();
    return remapped;
}

std::uint64_t
sooperlooper_clip_mapper::generation () const
{
    return m_generation;
}

int
sooperlooper_clip_mapper::clip_count () const
{
    return static_cast<int>(m_entries.size());
}

int
sooperlooper_clip_mapper::active_count () const
{
    int count = 0;
    for (const auto & e : m_entries)
    {
        if (e.active && e.generation == m_generation)
            ++count;
    }
    return count;
}

bool
sooperlooper_clip_mapper::is_registered (const std::string & uuid) const
{
    return m_uuid_to_index.count(uuid) > 0;
}

void
sooperlooper_clip_mapper::clear ()
{
    m_entries.clear();
    m_uuid_to_index.clear();
    m_index_to_uuid.clear();
    m_generation = 0;
}

const std::vector<clip_entry> &
sooperlooper_clip_mapper::entries () const
{
    return m_entries;
}

void
sooperlooper_clip_mapper::set_on_invalidate (
    std::function<void(std::uint64_t)> callback
)
{
    m_on_invalidate = callback;
}

void
sooperlooper_clip_mapper::rebuild_index_map ()
{
    m_uuid_to_index.clear();
    m_index_to_uuid.clear();

    for (int i = 0; i < static_cast<int>(m_entries.size()); ++i)
    {
        m_uuid_to_index[m_entries[i].uuid] = i;
        if (m_entries[i].active)
            m_index_to_uuid[m_entries[i].runtime_index] = m_entries[i].uuid;
    }
}

int
sooperlooper_clip_mapper::next_index () const
{
    int idx = 0;
    while (m_index_to_uuid.count(idx) > 0)
        ++idx;
    return idx;
}

} // namespace seq66

/*
 * sooperlooper_clip_mapper.cpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
