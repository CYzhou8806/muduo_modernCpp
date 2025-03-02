#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

/**
 * @brief Base class for I/O multiplexing
 * 
 * This class provides a uniform interface for different I/O multiplexing implementations
 * (like epoll, poll, etc). It manages the mapping between file descriptors and channels,
 * and provides methods for polling events.
 */
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;
    using ChannelMap = std::unordered_map<int, Channel*>;

    /**
     * @brief Constructs a Poller instance
     * @param inLoop The EventLoop that owns this Poller
     */
    explicit Poller(EventLoop *inLoop);
    virtual ~Poller() = default;

    /**
     * @brief Polls for I/O events
     * @param inTimeoutMs Maximum time to wait for events in milliseconds
     * @param outActiveChannels Output parameter to store channels with events
     * @return Timestamp when poll returns
     */
    virtual Timestamp poll(int inTimeoutMs, ChannelList *outActiveChannels) = 0;

    /**
     * @brief Updates a channel's events in the poller
     * @param inOutChannel The channel to update
     */
    virtual void updateChannel(Channel *inOutChannel) = 0;

    /**
     * @brief Removes a channel from the poller
     * @param inOutChannel The channel to remove
     */
    virtual void removeChannel(Channel *inOutChannel) = 0;
    
    /**
     * @brief Checks if a channel exists in this Poller
     * @param inChannel The channel to check
     * @return true if the channel exists, false otherwise
     */
    bool hasChannel(const Channel *inChannel) const;

    /**
     * @brief Creates a new default Poller instance
     * @param inLoop The EventLoop that will own the new Poller
     * @return A new Poller instance
     */
    static Poller* newDefaultPoller(EventLoop *inLoop);

protected:
    ChannelMap m_channels;  // Map of fd to Channel

private:
    EventLoop *m_ownerLoop;  // The EventLoop that owns this Poller
};