#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>    
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

namespace
{
[[nodiscard]] int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) 
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __func__, __LINE__, errno);
    }
    return sockfd;
}
}

Acceptor::Acceptor(EventLoop *inLoop, const InetAddress &inListenAddr, bool inReusePort)
    : m_loop(inLoop)
    , m_acceptSocket(createNonblocking())
    , m_acceptChannel(inLoop, m_acceptSocket.fd())
    , m_listenning(false)
{
    m_acceptSocket.setReuseAddr(true);
    m_acceptSocket.setReusePort(inReusePort);
    m_acceptSocket.bindAddress(inListenAddr);
    m_acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    m_acceptChannel.disableAll();
    m_acceptChannel.remove();
}

void Acceptor::listen()
{
    m_listenning = true;
    m_acceptSocket.listen();
    m_acceptChannel.enableReading();
}

void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connectionFd = m_acceptSocket.accept(&peerAddr);
    if (connectionFd >= 0)
    {
        if (m_newConnectionCallback)
        {
            m_newConnectionCallback(connectionFd, peerAddr);
        }
        else
        {
            ::close(connectionFd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __func__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __func__, __LINE__);
        }
    }
}
