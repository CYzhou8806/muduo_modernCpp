#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

/**
 * @brief A thread pool that manages multiple EventLoop threads
 * 
 * This class implements a thread pool specifically designed for EventLoop objects.
 * It maintains a base loop and creates additional loops running in separate threads.
 * Useful for multi-threaded event handling scenarios.
 */
class EventLoopThreadPool : noncopyable
{
public:
    /**
     * @brief Constructs an EventLoopThreadPool
     * 
     * @param inBaseLoop Pointer to the base EventLoop, typically the main loop
     * @param inNameArg Name of the thread pool for identification
     */
    explicit EventLoopThreadPool(EventLoop* inBaseLoop, const std::string& inNameArg);
    
    /**
     * @brief Destructor that ensures proper cleanup of all threads
     */
    ~EventLoopThreadPool();

    /**
     * @brief Sets the number of threads in the pool
     * Must be called before start()
     * 
     * @param inNumThreads Number of threads to create
     */
    void setThreadNum(int inNumThreads) { m_numThreads = inNumThreads; }

    /**
     * @brief Starts the thread pool
     * 
     * Creates and starts the specified number of EventLoopThread objects.
     * Each thread will run its own EventLoop.
     * 
     * @param inCallback the threadFunc that will be executed in the new threads
     */
    void start(const EventLoopThread::ThreadInitCallback &inCallback = EventLoopThread::ThreadInitCallback());

    /**
     * @brief Gets the next EventLoop in round-robin fashion
     * 
     * When working in multi-thread mode, the baseLoop assigns
     * channels to subloops in a round-robin manner.
     * 
     * @return EventLoop* Pointer to the next EventLoop to use
     */
    EventLoop* getNextLoop();

    /**
     * @brief Gets all EventLoop objects managed by the pool
     * 
     * @return std::vector<EventLoop*> Vector of pointers to all EventLoops
     */
    std::vector<EventLoop*> getAllLoops();

    /**
     * @brief Checks if the thread pool has been started
     * @return true if the pool has been started, false otherwise
     */
    bool started() const { return m_started; }

    /**
     * @brief Gets the name of the thread pool
     * @return The name of the thread pool
     */
    const std::string getName() const { return m_name; }

private:
    EventLoop* m_baseLoop;      // The main EventLoop, usually the one accepting connections
    std::string m_name;         // Name of the thread pool
    bool m_started;             // Flag indicating if the pool has been started
    int m_numThreads;           // Number of sub-threads in the pool
    int m_next;                 // Index for round-robin selection of EventLoops
    
    // Using unique_ptr for automatic resource management of threads
    std::vector<std::unique_ptr<EventLoopThread>> m_threads;
    std::vector<EventLoop*> m_loops; // as observer of loops
};
