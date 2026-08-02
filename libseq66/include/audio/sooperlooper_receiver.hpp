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
private:
    /** Forward declaration of the pimpl implementation. */
    class implementation;
    std::unique_ptr<implementation> m_impl;

public:
    /** Constructor. */
    explicit sooperlooper_receiver(int port = 0);

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

    /** Structure representing a received OSC event. */
    struct receiver_event
    {
        std::string path;                     ///< The OSC path of the message.
        std::string types;                    ///< The OSC type tag string.
        std::vector<std::string> args;        ///< The arguments as strings.
        long long timestamp_us;               ///< Timestamp in microseconds since epoch.
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
     *  Note: The callback is invoked from the liblo server thread, but the
     *        actual user-provided callback is executed via a queued mechanism
     *        to ensure it is safe for non-realtime contexts.
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
