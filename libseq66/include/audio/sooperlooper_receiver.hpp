/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_receiver.hpp
 *
 *  SooperLooper OSC receiver for handling incoming messages from the engine.
 */

#ifndef SEQ66_SOOPERLOOPER_RECEIVER_HPP
#define SEQ66_SOOPERLOOPER_RECEIVER_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

#if SEQ66_SOOPERLOOPER_SUPPORT
#include <lo/lo.h>
#endif

#include "audio/sooperlooper_protocol.hpp"

namespace seq66
{

/**
 *  SooperLooper OSC receiver.
 *
 *  This class manages a liblo server thread that listens for OSC messages
 *  from a SooperLooper engine. It provides a type-safe way to handle
 *  incoming messages by matching them against an allow-list of paths and
 *  argument signatures, and converting them to strongly-typed events.
 *
 *  The receiver is designed to be used from the main thread (or any thread)
 *  by polling for events. The callbacks from the liblo server thread are
 *  marshaled to a thread-safe queue to avoid real-time violations.
 */
class sooperlooper_receiver
{
    /// \note The start() and stop() methods are not thread-safe for concurrent calls from different threads.
    /// If concurrent start/stop is required, the caller must provide external synchronization (e.g., a mutex).
    /// However, the receiver is designed to be used from a single thread (e.g., the main or audio/UI thread).
    /// Calling start() while the receiver is already running returns true and has no effect (idempotent).
    /// Calling stop() when the receiver is already stopped is safe and has no effect.

private:
    /** Forward declaration of the pimpl implementation. */
    class implementation;
    std::unique_ptr<implementation> m_impl;

public:
    /**
     *  Constructor.
     *
     *  @param port           The UDP port to bind (0 lets the OS choose a free
     *                        port; the bound port is available via port()).
     *  @param max_queue_size The maximum number of queued events before
     *                        incoming messages are dropped (overflow counter
     *                        exposed via dropped_count()).
     */
    explicit sooperlooper_receiver(int port = 0, size_t max_queue_size = 4096);

    /** Destructor. */
    ~sooperlooper_receiver();

    // Disable copy and move
    sooperlooper_receiver(const sooperlooper_receiver &) = delete;
    sooperlooper_receiver & operator = (const sooperlooper_receiver &) = delete;

    /** Start the receiver thread. */
    bool start();

    /** Stop the receiver thread. */
    void stop();

    /** Check if the receiver is running. */
    bool started() const;

    /** Get the port the receiver is bound to (useful if port=0 was used). */
    int port() const;

    /** Number of messages dropped because the event queue was full. */
    unsigned long long dropped_count() const;

    /** Structure representing a received OSC event. */
    struct receiver_event
    {
        std::string path;                     ///< The OSC path of the message.
        std::string types;                    ///< The OSC type tag string.
        std::vector<std::string> args;        ///< The arguments as strings.
        long long timestamp_us;               ///< Timestamp in microseconds since epoch.
        std::uint64_t generation{0};          ///< Engine generation at receive time.
    };

    /**
     *  Register a handler for a specific OSC path and argument signature.
     *
     *  @param path   The OSC path to listen for (e.g., "/reply").
     *  @param types  The argument type string (e.g., "ss" for two strings).
     *  @param cb     The callback to invoke when a matching message is received.
     *                The callback receives the arguments as strings (for simplicity)
     *                and a timestamp in microseconds since the epoch.
     *
     *  @return true if the handler was registered successfully, false otherwise.
     *
     *  Note: Incoming messages are queued on the liblo server thread and the
     *        user-provided callback is executed later from dispatch() on the
     *        caller's thread. No user code runs on the liblo thread.
     */
    bool handle(const std::string & path, const std::string & types,
                std::function<void(const std::vector<std::string> & args,
                                   long long timestamp_us)> cb);

    /**
     *  Retrieve the next received event, if any.
     *
     *  This call is non-blocking: it returns immediately with false when the
     *  queue is empty.
     *
     *  @param[out] event  The event structure to fill.
     *  @return true if an event was available, false if the queue is empty.
     */
    bool poll_event(receiver_event & event);

    /**
     *  Block until the next event is available or the receiver stops.
     *
     *  @param[out] event  The event structure to fill.
     *  @return true if an event was available, false if the receiver stopped
     *          while waiting.
     */
    bool wait_event(receiver_event & event);

    /**
     *  Drain all queued events and invoke the registered handlers from the
     *  calling thread (a safe, non-realtime context).
     */
    void dispatch();
};

} // namespace seq66

#endif // SEQ66_SOOPERLOOPER_RECEIVER_HPP

/*
 * vim: sw=4 ts=4 wm=4 et ft=cpp
*/
