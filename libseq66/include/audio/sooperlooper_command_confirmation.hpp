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
    indeterminate,      /**< Deadline expired without confirmation.      */
    cancelled           /**< Operation cancelled by caller.              */
};

/**
 *  Result of a single command confirmation tracking entry.
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

    /** The loop index (or -1 for global). */
    int loop_index{-1};

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
 */
class command_confirmation_tracker
{
public:
    /** Callback for reconciliation queries. */
    using reconciliation_fn = std::function<bool(int loop_index)>;

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
     *  The callback receives a loop index and returns true if the
     *  loop is in the expected state (reconciliation successful).
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
     *  \param deadline_ms      Deadline in milliseconds from now.
     *  \return true if the operation was recorded.
     */
    bool track (const std::string & uuid,
                const std::string & description,
                const std::string & osc_path,
                int expected_state,
                int loop_index,
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
     *  \return Number of operations reconciled.
     */
    int reconcile ();

    /**
     *  Get the outcome of a specific operation.
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
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_COMMAND_CONFIRMATION_HPP

/*
 * sooperlooper_command_confirmation.hpp
 *
 * vim: sw=4 ts=4 wm=4 et ft=cpp
 */
