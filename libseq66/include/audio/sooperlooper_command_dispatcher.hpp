#if ! defined SEQ66_SOOPERLOOPER_COMMAND_DISPATCHER_HPP
#define SEQ66_SOOPERLOOPER_COMMAND_DISPATCHER_HPP

/*
 *  This file is part of Seq66 Loves SooperLooper.
 *
 *  Seq66 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */

/**
 * \file          sooperlooper_command_dispatcher.hpp
 *
 *  Performer audio command dispatch with desired/pending/observed lifecycle.
 *
 *  Dispatches audio commands (record, overdub, mute, etc.) through a
 *  three-phase lifecycle:
 *
 *    desired  → the performer requested a command
 *    pending  → the command was submitted to the engine and we await feedback
 *    confirmed/failed/indeterminate → terminal states
 *
 *  The dispatcher integrates:
 *  - clip UUID/runtime-index mapper for index resolution;
 *  - command confirmation tracker for deadline and verification;
 *  - observed state cache for reading engine feedback.
 *
 *  Thread safety: safe for single-writer (command sender) with external
 *  synchronization if multiple threads call dispatch simultaneously.
 */

#include <cstdint>
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "audio/sooperlooper_protocol.hpp"
#include "audio/sooperlooper_clip_mapper.hpp"
#include "audio/sooperlooper_command_confirmation.hpp"
#include "audio/sooperlooper_observed_state.hpp"

namespace seq66
{

/**
 *  Lifecycle status of a dispatched audio command.
 */
enum class command_status
{
    desired,            /**< Requested but not yet submitted to engine.    */
    pending,            /**< Submitted, awaiting confirmation.             */
    confirmed,          /**< Expected feedback observed.                   */
    failed,             /**< Engine reported an error.                     */
    indeterminate,      /**< Deadline expired; reconciliation negative.    */
    cancelled           /**< Cancelled by caller or crash invalidation.    */
};

/**
 *  A dispatched audio command.
 */
struct audio_command
{
    /** Unique command identifier (UUID). */
    std::string uuid;

    /** Clip UUID (resolved to runtime index by the mapper). */
    std::string clip_uuid;

    /** The command verb to send to the engine. */
    sooperlooper_command command;

    /** Runtime loop index resolved by the clip mapper (-1 = unresolved). */
    int runtime_index{-1};

    /** Engine generation when the command was created. */
    std::uint64_t generation{0};

    /** Current lifecycle status. */
    command_status status{command_status::desired};

    /** When the command was requested. */
    std::chrono::steady_clock::time_point requested_at;

    /** When the command was submitted to the engine. */
    std::chrono::steady_clock::time_point submitted_at;

    /** Deadline for confirmation after submission. */
    std::chrono::steady_clock::time_point deadline;

    /** Error message if failed. */
    std::string error;

    /** Human-readable description. */
    std::string description;
};

/**
 *  Callback types for command lifecycle events.
 */
using command_submit_fn =
    std::function<bool(int runtime_index, sooperlooper_command command)>;
using command_status_change_fn =
    std::function<void(const audio_command &)>;

/**
 *  Performer audio command dispatcher.
 *
 *  Integrates clip mapper, command confirmation tracker, and observed
 *  state cache into a single dispatch surface for performer-initiated
 *  audio commands.
 *
 *  Lifecycle:
 *  1. dispatch() creates an audio_command with status=desired.
 *  2. submit() resolves the clip UUID, calls the submit callback,
 *     registers with the confirmation tracker, and transitions to pending.
 *  3. check_feedback() queries the observed cache and confirms or
 *     marks commands as failed/indeterminate.
 *  4. on_generation_change() cancels all pending commands for the old
 *     generation.
 */
class sooperlooper_command_dispatcher
{
public:
    sooperlooper_command_dispatcher ();
    ~sooperlooper_command_dispatcher ();

    sooperlooper_command_dispatcher (
        const sooperlooper_command_dispatcher &
    ) = delete;
    sooperlooper_command_dispatcher & operator = (
        const sooperlooper_command_dispatcher &
    ) = delete;

    /**
     *  Set the callback used to submit commands to the engine.
     *
     *  The callback receives a resolved runtime index and command verb.
     *  It should send the OSC message and return true on success.
     */
    void set_submit_callback (command_submit_fn fn);

    /**
     *  Set the callback for command status changes.
     *
     *  Called whenever a command transitions lifecycle state.
     */
    void set_status_callback (command_status_change_fn fn);

    /**
     *  Set the clip mapper used for UUID-to-index resolution.
     *
     *  The dispatcher does NOT own the mapper.
     */
    void set_clip_mapper (sooperlooper_clip_mapper * mapper);

    /**
     *  Set the confirmation tracker used for deadline tracking.
     *
     *  The dispatcher does NOT own the tracker.
     */
    void set_confirmation_tracker (
        command_confirmation_tracker * tracker
    );

    /**
     *  Set the observed state cache for feedback queries.
     *
     *  The dispatcher does NOT own the cache.
     */
    void set_observed_cache (
        sooperlooper_observed_cache * cache
    );

    /**
     *  Dispatch a command to a clip.
     *
     *  Creates an audio_command with status=desired and stores it.
     *  The command is NOT submitted until submit_pending() is called.
     *
     *  \param clip_uuid     Clip UUID for index resolution.
     *  \param cmd           Command verb.
     *  \param description   Human-readable description.
     *  \param deadline_ms   Deadline for confirmation (ms from submission).
     *  \return Command UUID, or empty string on failure.
     */
    std::string dispatch (
        const std::string & clip_uuid,
        sooperlooper_command cmd,
        const std::string & description,
        int deadline_ms = 5000
    );

    /**
     *  Submit all desired commands to the engine.
     *
     *  Resolves clip UUIDs via the mapper, calls the submit callback,
     *  and transitions desired → pending.
     *
     *  \return Number of commands submitted.
     */
    int submit_pending ();

    /**
     *  Check observed feedback and confirm/reject pending commands.
     *
     *  Queries the observed state cache for each pending command's
     *  expected state transition.  Confirms if observed, marks as
     *  indeterminate if deadline expired.
     *
     *  \return Number of commands resolved (confirmed/failed/indeterminate).
     */
    int check_feedback ();

    /**
     *  Cancel a command by UUID.
     *
     *  Transitions to cancelled status.  Returns true if the command
     *  was found and in a cancellable state.
     */
    bool cancel (const std::string & uuid);

    /**
     *  Cancel all commands for a given engine generation.
     *
     *  Called on crash/restart to invalidate stale commands.
     *
     *  \return Number of commands cancelled.
     */
    int cancel_generation (std::uint64_t generation);

    /**
     *  Get a command by UUID (immutable snapshot).
     *
     *  Returns a copy; the internal command may be modified concurrently.
     *  Returns a default command with status=desired if not found.
     */
    audio_command command_by_uuid (const std::string & uuid) const;

    /**
     *  Get all commands in a specific status.
     */
    std::vector<audio_command> commands_by_status (
        command_status status
    ) const;

    /**
     *  Get all commands.
     */
    std::vector<audio_command> all_commands () const;

    /**
     *  Get the number of commands in a specific status.
     */
    int count_by_status (command_status status) const;

    /**
     *  Get the current engine generation.
     */
    std::uint64_t generation () const;

    /**
     *  Set the current engine generation.
     */
    void set_generation (std::uint64_t gen);

    /**
     *  Clear all commands.
     */
    void clear ();

private:
    mutable std::mutex m_mutex;
    std::uint64_t m_generation{0};
    std::map<std::string, audio_command> m_commands;
    sooperlooper_clip_mapper * m_mapper{nullptr};
    command_confirmation_tracker * m_tracker{nullptr};
    sooperlooper_observed_cache * m_cache{nullptr};
    command_submit_fn m_submit_fn;
    command_status_change_fn m_status_fn;

    void transition (
        audio_command & cmd,
        command_status new_status,
        const std::string & error = ""
    );
    std::string generate_uuid () const;
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_COMMAND_DISPATCHER_HPP

/*
 * sooperlooper_command_dispatcher.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
