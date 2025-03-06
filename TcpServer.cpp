#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <format>
#include <functional>

namespace {
    EventLoop* CheckLoopNotNull(EventLoop *inLoop)
    {
        if (inLoop == nullptr)
        {
            LOG_FATAL("{}:{}:{} mainLoop is null!\n", __FILE__, __func__, __LINE__);
        }
        return inLoop;
    }
}  // namespace

TcpServer::TcpServer(EventLoop *inLoop,
                     InetAddress inListenAddr,
                     std::string inNameArg,
                     Option inOption)
    : m_loop(CheckLoopNotNull(inLoop))
    , m_ipPort(inListenAddr.toIpPort())
    , m_name(std::move(inNameArg))
    , m_acceptor(std::make_unique<Acceptor>(inLoop, std::move(inListenAddr), inOption == Option::ReusePort))
    , m_threadPool(std::make_unique<EventLoopThreadPool>(inLoop, m_name))
    , m_nextConnId(1)
    , m_started(0)
{
    // Set new connection callback using lambda
    m_acceptor->setNewConnectionCallback(
        [this](int sockfd, const InetAddress& peerAddr) {
            newConnection(sockfd, peerAddr);
        });
}

TcpServer::~TcpServer() = default;

void TcpServer::setThreadNum(int inNumThreads)
{
    m_threadPool->setThreadNum(inNumThreads);
}

void TcpServer::start()
{
    if (m_started++ == 0)  // Prevent multiple starts
    {
        m_threadPool->start(m_threadInitCallback);
        m_loop->runInLoop([acceptor = m_acceptor.get()]() { acceptor->listen(); });
    }
}

void TcpServer::newConnection(int inSockfd, const InetAddress &inPeerAddr)
{
    // Get the next IO loop using round-robin
    EventLoop *ioLoop = m_threadPool->getNextLoop();
    
    // Create connection name
    std::string connName = std::format("{}-{}#{}", m_name, m_ipPort, m_nextConnId++);

    LOG_INFO("TcpServer::newConnection [{}] - new connection [{}] from {}\n",
             m_name, connName, inPeerAddr.toIpPort());

    // Get local address information
    sockaddr_in local{};
    socklen_t addrlen = sizeof(local);
    if (::getsockname(inSockfd, reinterpret_cast<sockaddr*>(&local), &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    // Create new TcpConnection
    auto conn = std::make_shared<TcpConnection>(
        ioLoop,
        std::move(connName), // This determines whether the parameter 'name' will be move-constructed or copy-constructed
        inSockfd,
        std::move(localAddr),
        inPeerAddr
    );

    // Store connection
    m_connections[conn->name()] = conn;

    // Set callbacks
    conn->setConnectionCallback(m_connectionCallback)
        .setMessageCallback(m_messageCallback)
        .setWriteCompleteCallback(m_writeCompleteCallback)
        .setCloseCallback([this](const TcpConnectionPtr& conn) { 
            removeConnection(conn); 
        });

    // Establish connection
    ioLoop->runInLoop([conn]() {
        conn->connectEstablished();
    });
}

void TcpServer::removeConnection(const TcpConnectionPtr &inConn)
{
    m_loop->runInLoop([this, conn = inConn]() {
        removeConnectionInLoop(conn);
    });
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &inConn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [{}] - connection {}\n",
             m_name, inConn->name());

    m_connections.erase(inConn->name());
    EventLoop *ioLoop = inConn->getLoop();
    
    ioLoop->queueInLoop([conn = inConn]() {
        conn->connectDestroyed();
    });
}
