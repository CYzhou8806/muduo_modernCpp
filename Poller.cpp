#include "Poller.h"
#include "Channel.h"
#include "EPollPoller.h"

#include <stdlib.h>

Poller::Poller(EventLoop *inLoop)
    : m_ownerLoop(inLoop)
{
}

bool Poller::hasChannel(const Channel *inChannel) const
{
    auto it = m_channels.find(inChannel->getFd());
    return it != m_channels.end() && it->second == inChannel;
}

Poller* Poller::newDefaultPoller(EventLoop *inLoop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        // TODO: Use poll(2) when requested by environment variable
        return nullptr;  // Poll implementation not available yet
    }
    else
    {
        // Use epoll(2) by default
        return new EPollPoller(inLoop);
    }
}
