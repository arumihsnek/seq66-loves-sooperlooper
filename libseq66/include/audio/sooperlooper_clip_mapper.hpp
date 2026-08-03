#if ! defined SEQ66_SOOPERLOOPER_CLIP_MAPPER_HPP
#define SEQ66_SOOPERLOOPER_CLIP_MAPPER_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_clip_mapper.hpp
 *
 *  Stable clip UUID to runtime-index mapper.
 *
 *  Clip identity is always the UUID.  Runtime indexes are ephemeral,
 *  generation-scoped, and invalidated on crash or topology change.
 *  The mapper never persists raw indexes as identity.
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace seq66
{

/**
 *  A clip entry in the mapper.
 */
struct clip_entry
{
    std::string uuid;               /**< Stable clip identity.           */
    int runtime_index{-1};          /**< Ephemeral engine loop index.    */
    std::uint64_t generation{0};    /**< Engine generation when assigned.*/
    bool active{false};             /**< Currently allocated in engine.  */
};

/**
 *  Pool allocation policy.
 */
enum class allocation_policy : int
{
    append_remove_last,     /**< Append new, remove last on deallocation.*/
    controlled_rebuild      /**< Full rebuild on topology change.        */
};

/**
 *  Clip UUID to runtime-index mapper.
 *
 *  Thread safety: all methods are safe to call from a single coordinator
 *  thread.  The caller must not call concurrent methods from multiple
 *  threads without external synchronization.
 */
class sooperlooper_clip_mapper
{
public:
    sooperlooper_clip_mapper ();
    ~sooperlooper_clip_mapper ();

    sooperlooper_clip_mapper (const sooperlooper_clip_mapper &) = delete;
    sooperlooper_clip_mapper & operator = (const sooperlooper_clip_mapper &) = delete;

    /**
     *  Set the allocation policy.
     */
    void set_allocation_policy (allocation_policy policy);

    /**
     *  Get the current allocation policy.
     */
    allocation_policy get_allocation_policy () const;

    /**
     *  Register a new clip by UUID.
     *
     *  Returns the assigned runtime index, or -1 if the UUID is already
     *  registered or the pool is full.
     */
    int register_clip (const std::string & uuid);

    /**
     *  Unregister a clip by UUID.
     *
     *  Returns true if the clip was found and removed.
     */
    bool unregister_clip (const std::string & uuid);

    /**
     *  Look up the runtime index for a UUID.
     *
     *  Returns -1 if the UUID is not registered or the generation
     *  does not match.
     */
    int lookup (const std::string & uuid) const;

    /**
     *  Look up the UUID for a runtime index.
     *
     *  Returns empty string if the index is not mapped in the current
     *  generation.
     */
    std::string reverse_lookup (int runtime_index) const;

    /**
     *  Invalidate all runtime indexes for the given generation.
     *
     *  Called on crash or topology change.  Clips remain registered
     *  but their runtime indexes become stale.
     */
    void invalidate_generation (std::uint64_t generation);

    /**
     *  Advance to a new generation, re-validating all active clips.
     *
     *  Returns the number of clips re-mapped.
     */
    int advance_generation (std::uint64_t new_generation);

    /**
     *  Get the current generation.
     */
    std::uint64_t generation () const;

    /**
     *  Get the number of registered clips.
     */
    int clip_count () const;

    /**
     *  Get the number of active (mapped) clips.
     */
    int active_count () const;

    /**
     *  Check if a UUID is registered.
     */
    bool is_registered (const std::string & uuid) const;

    /**
     *  Clear all clips and reset to initial state.
     */
    void clear ();

    /**
     *  Get all clip entries (for iteration/debugging).
     */
    const std::vector<clip_entry> & entries () const;

    /**
     *  Register a callback for generation invalidation.
     */
    void set_on_invalidate (
        std::function<void(std::uint64_t)> callback
    );

private:
    allocation_policy m_policy{allocation_policy::append_remove_last};
    std::uint64_t m_generation{0};
    std::vector<clip_entry> m_entries;
    std::unordered_map<std::string, int> m_uuid_to_index;
    std::unordered_map<int, std::string> m_index_to_uuid;
    std::function<void(std::uint64_t)> m_on_invalidate;

    void rebuild_index_map ();
    int next_index () const;
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_CLIP_MAPPER_HPP
