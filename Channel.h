#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

/**
 * Channel class manages the events and callbacks for a specific (fd) file descriptor.
 * It acts as a wrapper around the fd and its associated events (like EPOLLIN, EPOLLOUT).
 * 
 * The Channel class is responsible for:
 * 1. Managing event registration (read/write/error events)
 * 2. Handling event callbacks when events occur
 */ 
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    /**
     * @brief Constructs a new Channel instance
     * 
     * @param inLoop The EventLoop that owns this channel. Each channel must be used in its creator EventLoop thread
     * @param inFd The file descriptor to be monitored for I/O events
     */
    Channel(EventLoop *inLoop, int inFd);
    ~Channel();

    /**
     * @brief Handles events that occurred on the file descriptor
     * @param inReceiveTime The timestamp when the event was received
     * This function is called by the EventLoop when events occur on the fd
     */
    void handleEvent(Timestamp inReceiveTime);  

    /**
     * @brief Sets the callback function for read events
     * @param inCallback Function to be called when read events occur
     */
    void setReadCallback(ReadEventCallback inCallback) { m_readCallback = std::move(inCallback); }

    /**
     * @brief Sets the callback function for write events
     * @param inCallback Function to be called when write events occur
     */
    void setWriteCallback(EventCallback inCallback) { m_writeCallback = std::move(inCallback); }

    /**
     * @brief Sets the callback function for close events
     * @param inCallback Function to be called when close events occur
     */
    void setCloseCallback(EventCallback inCallback) { m_closeCallback = std::move(inCallback); }

    /**
     * @brief Sets the callback function for error events
     * @param inCallback Function to be called when error events occur
     */
    void setErrorCallback(EventCallback inCallback) { m_errorCallback = std::move(inCallback); }

    /**
     * @brief Ties this channel to the owner object's lifetime
     *
     * It is a optional operation. It will be used to tie a object which's lifetime is
     * managed by outside, like TCPConnection
     *
     * @param ptr Shared pointer to the owner object
     */
    void tie(const std::shared_ptr<void>&);

    /**
     * @brief Gets the file descriptor associated with this channel
     * @return The file descriptor
     */
    int getFd() const { return m_fd; }

    /**
     * @brief Gets the events that this channel is interested in
     * @return The events bitmap
     */
    int getEvents() const { return m_events; }

    /**
     * @brief Sets the events that actually occurred
     * @param inRevt The received events bitmap
     * @return The updated events bitmap
     */
    int setRevents(int inRevt) { m_revents = inRevt; return m_revents; }

    /**
     * @brief Enables reading events on this channel
     * Adds read event to the events bitmap and updates the channel
     */
    void enableReading() { m_events |= kReadEvent; update(); }

    /**
     * @brief Disables reading events on this channel
     * Removes read event from the events bitmap and updates the channel
     */
    void disableReading() { m_events &= ~kReadEvent; update(); }

    /**
     * @brief Enables writing events on this channel
     * Adds write event to the events bitmap and updates the channel
     */
    void enableWriting() { m_events |= kWriteEvent; update(); }

    /**
     * @brief Disables writing events on this channel
     * Removes write event from the events bitmap and updates the channel
     */
    void disableWriting() { m_events &= ~kWriteEvent; update(); }

    /**
     * @brief Disables all events on this channel
     * Clears all events from the bitmap and updates the channel
     */
    void disableAll() { m_events = kNoneEvent; update(); }

    /**
     * @brief Checks if no events are enabled
     * @return true if no events are enabled, false otherwise
     */
    bool isNoneEvent() const { return m_events == kNoneEvent; }

    /**
     * @brief Checks if write events are enabled
     * @return true if write events are enabled, false otherwise
     */
    bool isWriting() const { return m_events & kWriteEvent; }

    /**
     * @brief Checks if read events are enabled
     * @return true if read events are enabled, false otherwise
     */
    bool isReading() const { return m_events & kReadEvent; }

    /**
     * @brief Gets the index used by Poller to identify this channel
     * @return The channel's index in Poller
     */
    int getIndex() const { return m_index; }

    /**
     * @brief Sets the index used by Poller to identify this channel
     * @param idx The new index value
     */
    void setIndex(int idx) { m_index = idx; }

    /**
     * @brief Gets the EventLoop that owns this channel
     * @return Pointer to the owning EventLoop
     */
    EventLoop* ownerLoop() { return m_loop; }

    /**
     * @brief Removes this channel from its EventLoop
     * This operation will unregister all events and remove the channel from Poller
     */
    void remove();

private:
    /**
     * @brief Updates channel's events in EventLoop
     */
    void update();

    /**
     * @brief Handles events with a guard to prevent concurrent access
     * @param inReceiveTime The timestamp when the event was received
     */
    void handleEventWithGuard(Timestamp inReceiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *m_loop;    // The EventLoop that owns this channel
    const int m_fd;       // The file descriptor this channel is managing
    int m_events;         // Events that we're interested in, will be updated to epoll through the update() method
    int m_revents;        // Events that actually occurred from Poller
    int m_index;          // Used by Poller to track the channel

    std::weak_ptr<void> m_tie;  // Weak pointer to the owner object
    bool m_tied;                // Whether the channel is tied to an owner object

    // Event callbacks. These are called when corresponding events occur on the file descriptor
    ReadEventCallback m_readCallback;
    EventCallback m_writeCallback;
    EventCallback m_closeCallback;
    EventCallback m_errorCallback;
};
