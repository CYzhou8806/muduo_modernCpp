#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>

namespace
{
constexpr int kMaxListenQueueSize = 1024;

void setSocketOption(int sockfd, int level, int option, bool value)
{
    int optval = value ? 1 : 0;
    ::setsockopt(sockfd, level, option, &optval, sizeof(optval));
}
}  // namespace

Socket::~Socket()
{
    ::close(m_sockfd);
}

void Socket::bindAddress(const InetAddress &inLocaladdr)
{
    if (::bind(m_sockfd, static_cast<const sockaddr*>(inLocaladdr.getSockAddr()), 
               sizeof(sockaddr_in)) != 0)
    {
        LOG_FATAL(std::format("bind sockfd:{} fail", m_sockfd));
    }
}

void Socket::listen()
{
    if (::listen(m_sockfd, kMaxListenQueueSize) != 0)
    {
        LOG_FATAL(std::format("listen sockfd:{} fail", m_sockfd));
    }
}

int Socket::accept(InetAddress *inPeeraddr)
{
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    
    int connfd = ::accept4(m_sockfd, reinterpret_cast<sockaddr*>(&addr), 
                          &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    
    if (connfd >= 0)
    {
        inPeeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if (::shutdown(m_sockfd, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    setSocketOption(m_sockfd, IPPROTO_TCP, TCP_NODELAY, on);
}

void Socket::setReuseAddr(bool on)
{
    setSocketOption(m_sockfd, SOL_SOCKET, SO_REUSEADDR, on);
}

void Socket::setReusePort(bool on)
{
    setSocketOption(m_sockfd, SOL_SOCKET, SO_REUSEPORT, on);
}

void Socket::setKeepAlive(bool on)
{
    setSocketOption(m_sockfd, SOL_SOCKET, SO_KEEPALIVE, on);
}
