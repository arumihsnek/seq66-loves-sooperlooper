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
#include <atomic>

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

    std::cout << "Lifecycle and dispatch tests passed." << std::endl;

    // --- wait_event() blocking receive test ---
    {
        sooperlooper_receiver rx(0);
        if (!rx.start())
        {
            std::cerr << "ERROR: Failed to start receiver for wait_event test." << std::endl;
            return 1;
        }
        const int wait_port = rx.port();

        bool waiter_got_event = false;
        bool waiter_timed_out = false;
        std::thread waiter([&]()
        {
            sooperlooper_receiver::receiver_event wev;
            // Blocks until an event arrives; the 3s caller-side timeout below
            // guards against a hang if the implementation regresses.
            if (rx.wait_event(wev))
                waiter_got_event = (wev.path == "/wait");
            else
                waiter_timed_out = true;
        });

        // Give the waiter a moment to block, then send.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        lo_address wait_addr = lo_address_new_from_url
        (
            ("osc.udp://127.0.0.1:" + std::to_string(wait_port)).c_str()
        );
        if (!wait_addr)
        {
            std::cerr << "ERROR: Could not create OSC address for wait_event test." << std::endl;
            return 1;
        }
        lo_send(wait_addr, "/wait", "i", 1);
        lo_address_free(wait_addr);

        waiter.join();
        if (!waiter_got_event || waiter_timed_out)
        {
            std::cerr << "ERROR: wait_event() did not receive the test message." << std::endl;
            return 1;
        }
        rx.stop();
        std::cout << "wait_event() blocking receive passed." << std::endl;
    }

    // --- bounded queue overflow test ---
    {
        // Tiny queue: overflow must be counted, not grown without bound.
        sooperlooper_receiver rx(0, 4);
        if (!rx.start())
        {
            std::cerr << "ERROR: Failed to start receiver for overflow test." << std::endl;
            return 1;
        }
        const int overflow_port = rx.port();
        lo_address addr = lo_address_new_from_url
        (
            ("osc.udp://127.0.0.1:" + std::to_string(overflow_port)).c_str()
        );
        if (!addr)
        {
            std::cerr << "ERROR: Could not create OSC address for overflow test." << std::endl;
            return 1;
        }

        // Burst far more messages than the queue can hold without draining.
        for (int i = 0; i < 200; ++i)
            lo_send(addr, "/overflow", "i", i);
        lo_address_free(addr);

        // Give the receiver thread time to process the burst.
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        if (rx.dropped_count() == 0)
        {
            std::cerr << "ERROR: Overflow burst should have dropped messages, got 0." << std::endl;
            return 1;
        }

        // Drain what survived; the queue must never exceed the bound.
        size_t drained = 0;
        sooperlooper_receiver::receiver_event oev;
        while (rx.poll_event(oev))
            ++drained;
        if (drained > 4)
        {
            std::cerr << "ERROR: Queue exceeded its bound, drained " << drained << std::endl;
            return 1;
        }
        rx.stop();
        std::cout << "Bounded queue overflow test passed (drained " << drained
                  << ", dropped " << rx.dropped_count() << ")." << std::endl;
    }

    // --- concurrent handler registration and dispatch ---
    {
        sooperlooper_receiver rx(0);
        if (!rx.start())
        {
            std::cerr << "ERROR: Failed to start receiver for concurrency test." << std::endl;
            return 1;
        }
        const int conc_port = rx.port();

        std::atomic<int> received{0};
        std::atomic<bool> stop_flag{false};
        std::atomic<bool> reg_failed{false};

        // One thread keeps dispatching while another registers handlers.
        std::thread dispatcher([&]()
        {
            while (!stop_flag.load())
                rx.dispatch();
        });
        std::thread registrar([&]()
        {
            for (int i = 0; i < 50; ++i)
            {
                const std::string path = "/conc_" + std::to_string(i);
                if (!rx.handle(path, "i",
                    [&](const std::vector<std::string> &, long long)
                    {
                        received.fetch_add(1);
                    }))
                {
                    reg_failed.store(true);
                }
            }
        });

        // Send messages while the threads run.
        lo_address addr = lo_address_new_from_url
        (
            ("osc.udp://127.0.0.1:" + std::to_string(conc_port)).c_str()
        );
        if (!addr)
        {
            std::cerr << "ERROR: Could not create OSC address for concurrency test." << std::endl;
            return 1;
        }
        for (int round = 0; round < 10; ++round)
        {
            for (int i = 0; i < 50; ++i)
                lo_send(addr, ("/conc_" + std::to_string(i)).c_str(), "i", i);
        }
        lo_address_free(addr);

        registrar.join();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        stop_flag.store(true);
        dispatcher.join();
        rx.dispatch(); // final drain

        if (reg_failed.load())
        {
            std::cerr << "ERROR: Concurrent handler registration failed." << std::endl;
            return 1;
        }
        if (received.load() == 0)
        {
            std::cerr << "ERROR: Concurrency test received no messages." << std::endl;
            return 1;
        }
        rx.stop();
        std::cout << "Concurrent registration/dispatch passed (received "
                  << received.load() << ")." << std::endl;
    }

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
