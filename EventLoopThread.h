#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;

namespace muduoModernCpp
{
class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; 

    /**
     * @brief Constructs a new EventLoopThread object
     * 
     * Creates an event loop thread with optional initialization callback and name.
     * The thread is not started immediately after construction.
     * 
     * @param inCallback The callback function to be executed during thread initialization
     * @param inName The name of the thread (defaults to empty string)
     */
    EventLoopThread(const ThreadInitCallback &inCallback = ThreadInitCallback(), 
        const std::string &inName = std::string());

    /**
     * @brief Destructor for the EventLoopThread class
     * 
     * Handles cleanup of thread resources and ensures proper shutdown.
     * Sets the exiting flag and quits the event loop if it exists.
     * Joins the thread to ensure clean termination.
     */
    ~EventLoopThread();

    /**
     * @brief Starts the event loop thread
     * 
     * Creates and starts the thread with its own event loop.
     * Uses mutex and condition variable for synchronization to ensure
     * the event loop is fully initialized before returning.
     * 
     * @return EventLoop* Pointer to the created event loop
     */
    EventLoop* startLoop();

private:
    /**
     * @brief Thread execution function
     * 
     * Creates an event loop for the thread and runs it.
     * Executes the initialization callback if provided.
     * Uses synchronization to ensure proper startup sequence.
     */
    void threadFunc();

    EventLoop *m_loop;           // Pointer to the event loop owned by this thread
    bool m_exiting;             // Flag indicating whether the thread is exiting
    muduoModernCpp::Thread m_thread;            // The underlying thread object
    std::mutex m_mutex;         // Mutex for thread synchronization
    std::condition_variable m_cond;  // Condition variable for thread synchronization
    ThreadInitCallback m_callback;    // Callback function for thread initialization
};

} // namespace muduoModernCpp
