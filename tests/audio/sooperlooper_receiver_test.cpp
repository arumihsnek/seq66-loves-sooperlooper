/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          tests/audio/sooperlooper_receiver_test.cpp
 *
 *  Unit tests for the sooperlooper_receiver class.
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

#include "seq66-config.h"

#if SEQ66_SOOPERLOOPER_SUPPORT
#include <lo/lo.h>
#endif

#include "audio/sooperlooper_receiver.hpp"
#include "audio/sooperlooper_protocol.hpp"

using namespace seq66;

/**
 *  Test the receiver lifecycle and basic functionality.
 */
int main ()
{
    std::cout << "Testing sooperlooper_receiver..." << std::endl;

    // Create a receiver on port 0 (let the OS choose a port)
    sooperlooper_receiver receiver(0);

    // Check that it is not started initially
    if (receiver.started())
    {
        std::cerr << "ERROR: Receiver should not be started initially." << std::endl;
        return 1;
    }

    // Start the receiver
    if (!receiver.start())
    {
        std::cerr << "ERROR: Failed to start receiver." << std::endl;
        return 1;
    }

    // Check that it is now started
    if (!receiver.started())
    {
        std::cerr << "ERROR: Receiver should be started after calling start()." << std::endl;
        return 1;
    }

    // Get the port (should be > 0)
    int port = receiver.port();
    if (port <= 0)
    {
        std::cerr << "ERROR: Receiver port should be positive, got " << port << std::endl;
        return 1;
    }
    std::cout << "Receiver started on port " << port << std::endl;

    // Register a handler for a test path
    bool handler_called = false;
    std::string received_path;
    std::vector<std::string> received_args;
    long long received_timestamp = 0;

    auto test_handler = [&](const std::vector<std::string> & args, long long timestamp_us)
    {
        handler_called = true;
        received_args = args;
        received_timestamp = timestamp_us;
    };

    // Register a handler that captures the path for the poll path check.
    bool poll_path_ok = false;
    auto path_handler = [&](const std::vector<std::string> &, long long)
    {
        poll_path_ok = true;
    };

    const std::string test_path = "/test";
    const std::string test_types = "s"; // one string argument

    if (!receiver.handle(test_path, test_types, test_handler))
    {
        std::cerr << "ERROR: Failed to register handler for " << test_path << std::endl;
        return 1;
    }

    // Register a second path with an int argument to verify type matching.
    const std::string int_path = "/test_int";
    if (!receiver.handle(int_path, "i", path_handler))
    {
        std::cerr << "ERROR: Failed to register handler for " << int_path << std::endl;
        return 1;
    }

    // Duplicate registration must fail.
    if (receiver.handle(test_path, test_types, test_handler))
    {
        std::cerr << "ERROR: Duplicate handler registration should fail." << std::endl;
        return 1;
    }

    // Now, send a test OSC message to ourselves to see if we receive it.
#if SEQ66_SOOPERLOOPER_SUPPORT
    lo_address address = lo_address_new_from_url
    (
        ("osc.udp://127.0.0.1:" + std::to_string(port)).c_str()
    );
    if (!address)
    {
        std::cerr << "ERROR: Could not create OSC address for sending test message." << std::endl;
        return 1;
    }

    const char * test_string = "Hello, Receiver!";
    int result = lo_send(address, test_path.c_str(), test_types.c_str(), test_string);
    if (result < 0)
    {
        std::cerr << "ERROR: Failed to send test OSC message." << std::endl;
        lo_address_free(address);
        return 1;
    }

    result = lo_send(address, int_path.c_str(), "i", 42);
    lo_address_free(address);

    if (result < 0)
    {
        std::cerr << "ERROR: Failed to send int OSC message." << std::endl;
        return 1;
    }

    // Give some time for the messages to be processed and dispatched.
    // We use a bounded state-based wait instead of a long fixed sleep.
    const int max_attempts = 100;
    for (int attempt = 0; attempt < max_attempts; ++attempt)
    {
        receiver.dispatch();
        if (handler_called && poll_path_ok)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Check that our handlers were called (if we sent a message)
    if (!handler_called)
    {
        std::cerr << "ERROR: Handler was not called after sending a test message." << std::endl;
        return 1;
    }
    if (!poll_path_ok)
    {
        std::cerr << "ERROR: Int handler was not called after sending int message." << std::endl;
        return 1;
    }

    // Check that we received the expected arguments
    if (received_args.size() != 1 || received_args[0] != test_string)
    {
        std::cerr << "ERROR: Received unexpected arguments. Expected [" << test_string
                  << "], got [";
        for (const auto & arg : received_args)
            std::cerr << '"' << arg << "\" ";
        std::cerr << "]" << std::endl;
        return 1;
    }

    // Check that the timestamp is reasonable (not zero)
    if (received_timestamp == 0)
    {
        std::cerr << "ERROR: Received timestamp is zero." << std::endl;
        return 1;
    }

    std::cout << "Received message: path=" << test_path
              << ", args=[" << received_args[0]
              << "], timestamp=" << received_timestamp << std::endl;

    // Verify the poll_event path reports the OSC path of a raw message.
    // Send a message to a path WITHOUT a registered handler so it stays in
    // the queue (dispatch() consumed the handled messages above).
    lo_address raw_address = lo_address_new_from_url
    (
        ("osc.udp://127.0.0.1:" + std::to_string(port)).c_str()
    );
    if (!raw_address)
    {
        std::cerr << "ERROR: Could not create OSC address for raw poll test." << std::endl;
        return 1;
    }
    result = lo_send(raw_address, "/unhandled", "i", 7);
    lo_address_free(raw_address);
    if (result < 0)
    {
        std::cerr << "ERROR: Failed to send raw OSC message." << std::endl;
        return 1;
    }

    sooperlooper_receiver::receiver_event ev;
    bool found = false;
    for (int attempt = 0; attempt < max_attempts; ++attempt)
    {
        while (receiver.poll_event(ev))
        {
            if (ev.path == "/unhandled")
            {
                found = true;
                received_path = ev.path;
                break;
            }
        }
        if (found)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!found || received_path != "/unhandled")
    {
        std::cerr << "ERROR: poll_event did not report the expected path." << std::endl;
        return 1;
    }
    std::cout << "poll_event reported path=" << received_path << std::endl;
#endif

    // Test stopping the receiver
    receiver.stop();
    if (receiver.started())
    {
        std::cerr << "ERROR: Receiver should be stopped after calling stop()." << std::endl;
        return 1;
    }

    // Test that we can start it again (idempotent start/stop)
    if (!receiver.start())
    {
        std::cerr << "ERROR: Failed to restart receiver after stopping." << std::endl;
        return 1;
    }
    if (!receiver.started())
    {
        std::cerr << "ERROR: Receiver should be started after restart." << std::endl;
        return 1;
    }
    receiver.stop();

    // Verify poll_event is non-blocking on an empty queue after stop.
    if (receiver.poll_event(ev))
    {
        std::cerr << "ERROR: poll_event should return false after stop with empty queue." << std::endl;
        return 1;
    }

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
