/* 
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_command_confirmation.hpp
 *
 *  Command confirmation tracking for SooperLooper.
 *
 *  Tracks pending operations, expected feedback transitions, deadlines,
 *  and reconciliation.  OSC send success is never treated as user-visible
 *  completion; confirmation requires observed feedback or bounded
 *  verification.
 *
 *  M1-006B: Operations carry engine generation for stale detection.
 *  Reconciliation receives an immutable copy of the full operation.
 *  Unknown UUIDs return indeterminate, not cancelled.
 */

#ifndef SEQ66_SOOPERLOOPER_COMMAND_CONFIRMATION_HPP
#define SEQ66_SOOPERLOOPER_COMMAND_CONFIRMATION_HPP

#include <cstdint>
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace seq66
{

/**
 *  Outcome of a command confirmation attempt.
 */
enum class confirmation_outcome
{
    pending,            /**< Waiting for feedback.                       */
    confirmed,          /**< Expected transition observed.               */
    failed,             /**< Engine reported an error.                   */
    indeterminate,      /**< Deadline expired without confirmation,
                         *   or UUID not found.                         */
    cancelled           /**< Operation cancelled by caller.              */
};

/**
 *  Result of a single command confirmation tracking entry.
 *
 *  M1-006B: Added engine_generation, expected_control, and request_uuid
 *  for generation-aware reconciliation and typed verification.
 */
struct pending_operation
{
    /** UUID identifying this operation. */
    std::string uuid;

    /** Human-readable description (e.g. "record loop 0"). */
    std::string description;

    /** The OSC path that was sent. */
    std::string osc_path;

    /** The expected state value after confirmation (or -1 for any). */
    int expected_state{-1};

    /** The expected control name for verification (empty = any). */
    std::string expected_control;

    /** The loop index (or -1 for global). */
    int loop_index{-1};

    /** Engine generation when this operation was created. */
    std::uint64_t engine_generation{0};

    /** When the operation was sent. */
    std::chrono::steady_clock::time_point sent_at;

    /** Deadline for confirmation. */
    std::chrono::steady_clock::time_point deadline;

    /** Current outcome. */
    confirmation_outcome outcome{confirmation_outcome::pending};

    /** Error message if failed. */
    std::string error;
};

/**
 *  Command confirmation tracker.
 *
 *  Tracks pending operations and their expected feedback transitions.
 *  Automatic reconciliation occurs after deadlines expire.  The tracker
 *  is thread-safe for single-writer (command sender) and multiple-reader
 *  (UI/monitor) access.
 *
 *  Design:
 *  - Operations are identified by UUID (unique across restarts).
 *  - Each operation has an expected state transition and a deadline.
 *  - When feedback arrives, it is correlated by loop_index + expected_state.
 *  - After deadline expiry, the operation transitions to indeterminate.
 *  - Reconciliation can be triggered explicitly or automatically.
 *  - M1-006B: Operations carry engine generation; stale operations
 *    after restart are not confirmed.
 */
class command_confirmation_tracker
{
public:

    /**
     *  Callback for reconciliation queries.
     *
     *  M1-006B: Receives an immutable copy of the pending operation
     *  instead of just the loop index.  This allows the reconciler to
     *  verify the expected state, control name, generation, and other
     *  fields before confirming.
     *
     *  The reconciler must NOT modify the operation.  It should query
     *  the engine (or observed state cache) and return true only if
     *  the transition was actually observed.
     *
     *  Returns true if the operation's expected transition was observed.
     */
    using reconciliation_fn = std::function<bool(const pending_operation &)>;

private:

    /** Mutex protecting all internal state. */
    mutable std::mutex m_mutex;

    /** Pending operations keyed by UUID. */
    std::map<std::string, pending_operation> m_operations;

    /** Callback for reconciliation queries. */
    reconciliation_fn m_reconciler;

public:

    command_confirmation_tracker () = default;
    ~command_confirmation_tracker () = default;

    // Disable copy and move.
    command_confirmation_tracker (const command_confirmation_tracker &) = delete;
    command_confirmation_tracker & operator = (const command_confirmation_tracker &) = delete;

    /**
     *  Set the reconciliation callback.
     *
     *  M1-006B: The callback receives an immutable copy of the pending
     *  operation.  It must verify the expected state, control, and
     *  generation before returning true.
     */
    void set_reconciler (reconciliation_fn fn);

    /**
     *  Record a new pending operation.
     *
     *  \param uuid             Unique operation identifier.
     *  \param description      Human-readable description.
     *  \param osc_path         The OSC path that was sent.
     *  \param expected_state   Expected state after confirmation (-1 = any).
     *  \param loop_index       Loop index (-1 = global).
     *  \param engine_generation Engine generation when operation was created.
     *  \param expected_control Expected control name (empty = any).
     *  \param deadline_ms      Deadline in milliseconds from now.
     *  \return true if the operation was recorded.
     */
    bool track (const std::string & uuid,
                const std::string & description,
                const std::string & osc_path,
                int expected_state,
                int loop_index,
                std::uint64_t engine_generation = 0,
                const std::string & expected_control = "",
                int deadline_ms = 5000);

    /**
     *  Confirm an operation by observing the expected state.
     *
     *  \param loop_index   The loop that changed.
     *  \param new_state    The observed state value.
     *  \return true if a pending operation was confirmed.
     */
    bool confirm (int loop_index, int new_state);

    /**
     *  Explicitly confirm an operation by UUID.
     */
    bool confirm_by_uuid (const std::string & uuid);

    /**
     *  Mark an operation as failed with an error message.
     */
    bool fail (const std::string & uuid, const std::string & error);

    /**
     *  Cancel a pending operation.
     */
    bool cancel (const std::string & uuid);

    /**
     *  Evaluate deadlines and transition expired operations to
     *  indeterminate.  Optionally trigger reconciliation.
     *
     *  \return Number of operations transitioned.
     */
    int evaluate ();

    /**
     *  Trigger reconciliation for all indeterminate operations.
     *
     *  M1-006B: The reconciler receives an immutable copy of each
     *  operation.  After the callback returns, the tracker re-acquires
     *  the lock and verifies the operation hasn't changed (generation,
     *  outcome) before confirming.
     *
     *  \return Number of operations reconciled.
     */
    int reconcile ();

    /**
     *  Get the outcome of a specific operation.
     *
     *  M1-006B: Returns indeterminate for unknown UUID (not cancelled).
     */
    confirmation_outcome outcome (const std::string & uuid) const;

    /**
     *  Get all pending operations.
     */
    std::vector<pending_operation> pending () const;

    /**
     *  Get all operations with a specific outcome.
     */
    std::vector<pending_operation> by_outcome (confirmation_outcome o) const;

    /**
     *  Return the number of pending operations.
     */
    std::size_t pending_count () const;

    /**
     *  Clear all operations.
     */
    void clear ();

    /**
     *  Cancel all operations for a given engine generation.
     *
     *  M1-006B: Used during engine restart to invalidate stale
     *  operations from the previous generation.
     *
     *  \param generation   The generation whose operations to cancel.
     *  \return Number of operations cancelled.
     */
    int cancel_generation (std::uint64_t generation);
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_COMMAND_CONFIRMATION_HPP

/*
 * sooperlooper_command_confirmation.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
