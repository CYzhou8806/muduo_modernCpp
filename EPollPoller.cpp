#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

// Channel status constants
const int kStatusNew = -1;     // Channel not added to poller
const int kStatusAdded = 1;    // Channel added to poller
const int kStatusDeleted = 2;  // Channel deleted from poller

EPollPoller::EPollPoller(EventLoop *inLoop)
    : Poller(inLoop)
    , m_epollfd(::epoll_create1(EPOLL_CLOEXEC))
    , m_events(kInitEventListSize)
{
    if (m_epollfd < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller() 
{
    ::close(m_epollfd);
}

void EPollPoller::update(int inOperation, Channel *inChannel)
{
    epoll_event event;
    bzero(&event, sizeof event);
    
    int fd = inChannel->fd();

    event.events = inChannel->events();
    event.data.fd = fd; 
    event.data.ptr = inChannel;
    
    if (::epoll_ctl(m_epollfd, inOperation, fd, &event) < 0)
    {
        if (inOperation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}

Timestamp EPollPoller::poll(int inTimeoutMs, ChannelList *outActiveChannels)
{
    LOG_INFO("func=%s => fd total count:%lu \n", __func__, m_channels.size());

    int numEvents = ::epoll_wait(m_epollfd, &*m_events.begin(), static_cast<int>(m_events.size()), inTimeoutMs);
    // Save errno immediately after system call as it might be modified by subsequent operations
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, outActiveChannels);
        if (numEvents == m_events.size())
        {
            m_events.resize(m_events.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __func__);
    }
    else  // numEvents < 0 indicates an error in epoll_wait
    {
        // EINTR is not a real error, it means the system call was interrupted by a signal
        // In this case, we can simply retry the call, so we don't log an error
        if (saveErrno != EINTR)
        {
            // Restore the original errno before logging as the log function might change it
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() error: %d: %s", errno, strerror(errno));
        }
    }
    // Always return current timestamp regardless of whether we got events or errors
    return now;
}

void EPollPoller::updateChannel(Channel *inOutChannel)
{
    const int status = inOutChannel->getChannelStatus();
    LOG_INFO("func=%s => fd=%d events=%d status=%d \n", __func__, inOutChannel->fd(), inOutChannel->events(), status);

    if (status == kStatusNew || status == kStatusDeleted)
    {
        if (status == kStatusNew)
        {
            int fd = inOutChannel->fd();
            m_channels[fd] = inOutChannel;
        }

        inOutChannel->setChannelStatus(kStatusAdded);
        update(EPOLL_CTL_ADD, inOutChannel);
    }
    else  // Channel is already registered in epoll
    {
        int fd = inOutChannel->fd();
        if (inOutChannel->isNoneEvent())  // Check if the channel is not interested in any events
        {
            update(EPOLL_CTL_DEL, inOutChannel);
            inOutChannel->setChannelStatus(kStatusDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, inOutChannel);
        }
    }
}

void EPollPoller::removeChannel(Channel *inOutChannel) 
{
    int fd = inOutChannel->fd();
    m_channels.erase(fd);

    LOG_INFO("func=%s => fd=%d\n", __func__, fd);
    
    int status = inOutChannel->getChannelStatus();
    if (status == kStatusAdded)
    {
        update(EPOLL_CTL_DEL, inOutChannel);
    }
    inOutChannel->setChannelStatus(kStatusNew);
}

void EPollPoller::fillActiveChannels(int inNumEvents, ChannelList *outActiveChannels) const
{
    for (int i=0; i < inNumEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(m_events[i].data.ptr);
        channel->set_revents(m_events[i].events);
        outActiveChannels->push_back(channel);
    }
}
