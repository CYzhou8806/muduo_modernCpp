#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *inLoop, int inFd)
    : m_loop(inLoop), m_fd(inFd), m_events(0), m_revents(0), m_channelStatus(-1), m_tied(false)
{
}

Channel::~Channel()
{
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
    m_tie = obj;
    m_tied = true;
}


void Channel::update()
{
    // Through the channel's owning EventLoop, call the poller's corresponding method to register fd events
    m_loop->updateChannel(this);
}

void Channel::remove()
{
    m_loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp inReceiveTime)
{
    if (m_tied)
    {
        std::shared_ptr<void> guard = m_tie.lock();
        if (guard)
        {
            handleEventWithGuard(inReceiveTime);
        }
    }
    else
    {
        handleEventWithGuard(inReceiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp inReceiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", m_revents);

    if ((m_revents & EPOLLHUP) && !(m_revents & EPOLLIN))
    {
        if (m_closeCallback)
        {
            m_closeCallback();
        }
    }

    if (m_revents & EPOLLERR)
    {
        if (m_errorCallback)
        {
            m_errorCallback();
        }
    }

    if (m_revents & (EPOLLIN | EPOLLPRI))
    {
        if (m_readCallback)
        {
            m_readCallback(inReceiveTime);
        }
    }

    if (m_revents & EPOLLOUT)
    {
        if (m_writeCallback)
        {
            m_writeCallback();
        }
    }
}
