#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;

/**
 * @brief Implementation of Poller using epoll
 * 
 * This class implements the I/O multiplexing interface using Linux's epoll mechanism.
 * It manages the lifecycle of an epoll instance and provides three main operations:
 * - epoll_create: Creates the epoll instance
 * - epoll_ctl: Adds, modifies, or removes file descriptors
 * - epoll_wait: Waits for I/O events
 */ 
class EPollPoller : public Poller
{
public:
    /**
     * @brief Constructs an EPollPoller instance
     * @param inLoop The EventLoop that owns this poller
     */
    EPollPoller(EventLoop *inLoop);

    /**
     * @brief Destructor that closes the epoll file descriptor
     */
    ~EPollPoller() override;

    /**
     * @brief Polls for I/O events
     * @param inTimeoutMs Maximum time to wait for events in milliseconds
     * @param outActiveChannels Output parameter to store channels with events
     * @return Timestamp when poll returns
     */
    Timestamp poll(int inTimeoutMs, ChannelList *outActiveChannels) override;
    void updateChannel(Channel *inOutChannel) override;
    void removeChannel(Channel *inOutChannel) override;

private:
    static const int kInitEventListSize = 16;  // Initial size of events array

    /**
     * @brief Updates epoll control for a channel
     * @param inOperation The operation to perform (EPOLL_CTL_*)
     * @param inChannel The channel to operate on
     */
    void update(int inOperation, Channel *inChannel);

    /**
     * @brief Fills the active channels list from epoll events
     * @param inNumEvents Number of events returned by epoll_wait
     * @param outActiveChannels List to store active channels
     */
    void fillActiveChannels(int inNumEvents, ChannelList *outActiveChannels) const;

    using EventList = std::vector<epoll_event>;

    int m_epollfd;           // The epoll file descriptor
    EventList m_events;      // Storage for epoll events
};