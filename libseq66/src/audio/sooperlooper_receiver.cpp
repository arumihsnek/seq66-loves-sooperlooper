/*
 *  This file is part of Seq66 Loves SooperLooper.
 */

/**
 *  \file          sooperlooper_receiver.cpp
 *
 *  SooperLooper OSC receiver for handling incoming messages from the engine.
 */

#include "audio/sooperlooper_receiver.hpp"

#include "seq66-config.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if SEQ66_SOOPERLOOPER_SUPPORT
#include <lo/lo.h>
#endif

#include "audio/sooperlooper_protocol.hpp"

namespace seq66
{

class sooperlooper_receiver::implementation
{
private:
    /** The liblo server thread. */
#if SEQ66_SOOPERLOOPER_SUPPORT
    lo_server_thread m_server_thread;
#else
    int m_server_thread; // dummy to keep size
#endif
    /** The port the server is bound to. */
    int m_port;
    /** Port string passed to liblo (persistent, since liblo keeps the pointer). */
    std::string m_port_str;
    /** Flag indicating whether the server thread is running. */
    std::atomic<bool> m_running{false};
    /** Mutex for protecting the event queue. */
    std::mutex m_queue_mutex;
    /** Condition variable for waiting on the event queue. */
    std::condition_variable m_queue_cv;
    /** Queue of received events. */
    std::queue<receiver_event> m_event_queue;
    /** Map of (path, types) to callback. */
    struct handler_key
    {
        std::string path;
        std::string types;
        bool operator==(const handler_key & other) const
        {
            return path == other.path && types == other.types;
        }
    };
    struct handler_key_hash
    {
        size_t operator()(const handler_key & k) const
        {
            return std::hash<std::string>()(k.path) ^
                   (std::hash<std::string>()(k.types) << 1);
        }
    };
    std::unordered_map<handler_key,
                       std::function<void(const std::vector<std::string> &,
                                          long long)>,
                       handler_key_hash> m_handlers;

    /** Static trampoline function for liblo. */
    static int trampoline(const char * path, const char * types,
                          lo_arg ** argv, int argc,
                          void * data, void * user_data)
    {
        (void) data;
        auto * self = static_cast<implementation *>(user_data);
        if (!self || !self->m_running.load())
            return 0;

        // Convert the lo_arg arguments to strings.
        std::vector<std::string> args;
        args.reserve(argc);
        for (int i = 0; i < argc; ++i)
        {
            switch (types[i])
            {
                case 'i':
                    args.push_back(std::to_string(argv[i]->i));
                    break;
                case 'f':
                    args.push_back(std::to_string(argv[i]->f));
                    break;
                case 's':
                case 'S':
                    {
                        // liblo stores strings/symbols inline: read via &argv[i]->s.
                        const char * str = &argv[i]->s;
                        if (str)
                            args.push_back(str);
                        else
                            args.push_back("");
                    }
                    break;
                default:
                    // Unsupported type; treat as empty string.
                    args.push_back("");
                    break;
            }
        }

        // Marshal to the event queue: never run user code on the liblo thread.
        {
            std::lock_guard<std::mutex> lock(self->m_queue_mutex);
            self->m_event_queue.emplace(receiver_event
            {
                path,
                types,
                std::move(args),
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()
            });
        }
        self->m_queue_cv.notify_one();

        // Return 0 to indicate we consumed the message.
        return 0;
    }

public:
    explicit implementation(int port = 0) : m_port(port), m_port_str(std::to_string(port)) {}

    ~implementation()
    {
        stop();
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (m_server_thread)
            lo_server_thread_free(m_server_thread);
#endif
    }

    bool start()
    {
        if (m_running.load())
            return true;

#if SEQ66_SOOPERLOOPER_SUPPORT
        // Create the server thread. Passing nullptr lets the OS choose a free
        // port; otherwise pass the explicit port as a string.
        const char * port_arg = (m_port == 0) ? nullptr : m_port_str.c_str();
        m_server_thread = lo_server_thread_new(port_arg, error_handler);
        if (!m_server_thread)
            return false;

        // Register the catch-all trampoline. Every incoming message is queued;
        // dispatch() later filters by the exact (path, types) allow-list.
        // Using a catch-all means unknown controls remain observable via
        // poll_event() instead of being silently dropped by liblo.
        lo_server_thread_add_method(m_server_thread, nullptr, nullptr, trampoline, this);

        // Get the actual port bound (when the OS chose it).
        lo_server srv = lo_server_thread_get_server(m_server_thread);
        if (srv)
        {
            m_port = lo_server_get_port(srv);
            m_port_str = std::to_string(m_port);
        }
        if (m_port <= 0)
        {
            lo_server_thread_free(m_server_thread);
            m_server_thread = nullptr;
            return false;
        }

        // Start the thread.
        if (lo_server_thread_start(m_server_thread) < 0)
        {
            lo_server_thread_free(m_server_thread);
            m_server_thread = nullptr;
            return false;
        }
#else
        (void) m_port;
#endif
        m_running.store(true);
        return true;
    }

    void stop()
    {
        if (!m_running.load())
            return;

        m_running.store(false);
#if SEQ66_SOOPERLOOPER_SUPPORT
        if (m_server_thread)
        {
            lo_server_thread_stop(m_server_thread);
            lo_server_thread_free(m_server_thread);
            m_server_thread = nullptr;
        }
#endif
        // Wake up any thread waiting in wait_event().
        m_queue_cv.notify_all();
    }

    bool started() const
    {
        return m_running.load();
    }

    int port() const
    {
        return m_port;
    }

    bool handle(const std::string & path, const std::string & types,
                std::function<void(const std::vector<std::string> & args,
                                   long long timestamp_us)> cb)
    {
        // Disallow empty path or types.
        if (path.empty() || types.empty())
            return false;

        // Prevent overwriting an existing handler for the same (path, types).
        handler_key key{path, types};
        if (m_handlers.find(key) != m_handlers.end())
            return false;

        m_handlers.emplace(key, std::move(cb));
        return true;
    }

    bool poll_event(receiver_event & event)
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        if (m_event_queue.empty())
            return false;

        event = m_event_queue.front();
        m_event_queue.pop();
        return true;
    }

    /** Block until an event is available or the receiver stops. */
    bool wait_event(receiver_event & event)
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_queue_cv.wait(lock, [this] { return !m_event_queue.empty() || !m_running.load(); });
        if (m_event_queue.empty())
            return false;

        event = m_event_queue.front();
        m_event_queue.pop();
        return true;
    }

    /** Drain queued events and invoke their handlers from a safe context. */
    void dispatch()
    {
        receiver_event event;
        while (poll_event(event))
        {
            handler_key key{event.path, event.types};
            auto it = m_handlers.find(key);
            if (it != m_handlers.end() && it->second)
            {
                it->second(event.args, event.timestamp_us);
            }
        }
    }

    /** Static error handler for liblo. */
    static void error_handler(int err_num, const char * msg, const char * path)
    {
        // We could log this, but for now we just ignore it.
        (void) err_num;
        (void) msg;
        (void) path;
    }
};

sooperlooper_receiver::sooperlooper_receiver(int port)
    : m_impl(std::make_unique<implementation>(port))
{
}

sooperlooper_receiver::~sooperlooper_receiver() = default;

bool sooperlooper_receiver::start()
{
    return m_impl->start();
}

void sooperlooper_receiver::stop()
{
    m_impl->stop();
}

bool sooperlooper_receiver::started() const
{
    return m_impl->started();
}

int sooperlooper_receiver::port() const
{
    return m_impl->port();
}

bool sooperlooper_receiver::handle(const std::string & path, const std::string & types,
                                   std::function<void(const std::vector<std::string> & args,
                                                      long long timestamp_us)> cb)
{
    return m_impl->handle(path, types, std::move(cb));
}

bool sooperlooper_receiver::poll_event(receiver_event & event)
{
    return m_impl->poll_event(event);
}

void sooperlooper_receiver::dispatch()
{
    m_impl->dispatch();
}

} // namespace seq66

/*
 * vim: sw=4 ts=4 wm=4 et ft=cpp
*/
