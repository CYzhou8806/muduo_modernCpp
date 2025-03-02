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
     * This function is called by the EventLoop when events occur on the fd
     * @brief Handles events that occurred on the file descriptor
     * @param inReceiveTime The timestamp when the event was received
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
     */
    void enableReading() { m_events |= kReadEvent; update(); }

    /**
     * @brief Disables reading events on this channel
     */
    void disableReading() { m_events &= ~kReadEvent; update(); }

    /**
     * @brief Enables writing events on this channel
     */
    void enableWriting() { m_events |= kWriteEvent; update(); }

    /**
     * @brief Disables writing events on this channel
     */
    void disableWriting() { m_events &= ~kWriteEvent; update(); }

    /**
     * @brief Disables all events on this channel
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
     * @brief Gets the current status of this channel in the event system
     * @return The channel's current status
     */
    int getChannelStatus() const { return m_channelStatus; }

    /**
     * @brief Sets the current status of this channel in the event system
     * @param status The new status value
     */
    void setChannelStatus(int status) { m_channelStatus = status; }

    /**
     * @brief Removes this channel from its EventLoop
     */
    void remove();

    /**
     * @brief Gets the EventLoop that owns this channel
     * @return Pointer to the owning EventLoop
     */
    EventLoop* ownerLoop() { return m_loop; }

private:
    /**
     * @brief Updates channel's events in EventLoop
     */
    void update();

    /**
     * @brief Handles events with a guard to prevent concurrent access
     */
    void handleEventWithGuard(Timestamp inReceiveTime);

    // Constants for event types
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *m_loop;              // The EventLoop that owns this channel
    const int m_fd;                 // The file descriptor
    int m_events;                   // Events of interest
    int m_revents;                  // Events that actually occurred
    int m_channelStatus;            // Current status in the event system
    std::weak_ptr<void> m_tie;      // Weak pointer to the owner object
    bool m_tied;                    // Whether the channel is tied to an owner

    // Event callbacks
    ReadEventCallback m_readCallback;
    EventCallback m_writeCallback;
    EventCallback m_closeCallback;
    EventCallback m_errorCallback;
};
