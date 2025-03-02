#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "Channel.h"
#include "Poller.h"

#include <functional>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <unistd.h>     // for syscall
#include <sys/syscall.h> // for SYS_gettid

/**
 * @brief Event Loop class for handling I/O events
 * 
 * This class consists of two main components:
 * - Channel: Responsible for event dispatching
 * - Poller: An abstraction of epoll for I/O multiplexing
 */
class EventLoop : noncopyable {
public:
    using ChannelList = std::vector<Channel *>;
    using Functor = std::function<void()>;

    EventLoop();

    ~EventLoop();

    /**
     * @brief Starts the event loop
     * 
     * The main event processing loop that:
     * 1. Waits for events using Poller
     * 2. Handles active channels
     * 3. Executes pending callbacks
     */
    void loop();

    /**
     * @brief Quits the event loop
     * 
     * Sets the quit flag to true and wakes up the loop
     */
    void quit();

    /**
     * @brief Gets the timestamp of the last poll return
     */
    Timestamp pollReturnTime() const { return m_pollReturnTime; }

    /**
     * @brief Runs callback in the loop thread
     * 
     * If called from the loop thread, executes immediately
     * If called from other threads, queues the callback
     */
    void runInLoop(Functor inCallback);

    /**
     * @brief Queues callback in the loop thread
     * 
     * Thread-safe method to queue callback and wake up the loop
     * The callback will be executed by the loop thread
     */
    void queueInLoop(Functor inCallback);

    /**
     * @brief Wakes up the loop thread
     * 
     * Writes to the eventfd to wake up epoll_wait
     */
    void wakeup();

    /**
     * @brief Channel operations that delegate to Poller
     * 
     * These methods manage the relationship between Channels and Poller
     */
    void updateChannel(Channel *inChannel); // Updates channel's events in poller
    void removeChannel(Channel *inChannel); // Removes channel from poller
    bool hasChannel(Channel *inChannel); // Checks if channel exists in current loop

    /**
     * @brief Checks if current thread is the loop thread
     * 
     * Compares current thread id with the thread id that created this EventLoop
     */
    bool isInLoopThread() const { return m_threadId == ::syscall(SYS_gettid); }

private:
    /**
     * @brief Handles wakeup event
     * Reads from eventfd when loop is woken up
     */
    void handleRead(); // wake up

    /**
     * @brief Executes pending callbacks
     * Processes all callbacks in the pending queue
     */
    void doPendingFunctors();

    std::atomic_bool m_looping;
    std::atomic_bool m_quit;
    const pid_t m_threadId;

    Timestamp m_pollReturnTime; // Records when the poller last returned with active events
    std::unique_ptr<Poller> m_poller; // Manages the lifetime of the Poller object

    /**
     * @brief File descriptor for waking up the event loop
     * Used when mainLoop assigns a new client channel to a subloop
     * through round-robin scheduling, this fd is used to wake up 
     * the target subloop to handle the new channel
     */
    int m_wakeupFd;

    /**
     * @brief Channel for handling wakeup events
     * Wraps the wakeupFd for event handling
     */
    std::unique_ptr<Channel> m_wakeupChannel;

    ChannelList m_activeChannels; // Stores channels that have pending events to process
    std::atomic_bool m_callingPendingFunctors;
    std::vector<Functor> m_pendingFunctors; // Stores callbacks that need to be executed in the loop thread
    std::mutex m_mutex;
};
