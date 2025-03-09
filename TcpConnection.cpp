#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <string_view>

namespace {
    constexpr size_t kDefaultHighWaterMark = 64 * 1024 * 1024;  // 64MB

    EventLoop* CheckLoopNotNull(EventLoop *inLoop)
    {
        if (inLoop == nullptr)
        {
            LOG_FATAL("%s:%s:%d TcpConnection Loop is null!\n", __FILE__, __func__, __LINE__);
        }
        return inLoop;
    }
}  // namespace

TcpConnection::TcpConnection(EventLoop *inLoop, 
                           std::string inName, 
                           int inSockfd,
                           InetAddress inLocalAddr,
                           InetAddress inPeerAddr)
    : m_loop(CheckLoopNotNull(inLoop))
    , m_name(std::move(inName))
    , m_state(State::Connecting)
    , m_reading(true)
    , m_socket(std::make_unique<Socket>(inSockfd))
    , m_channel(std::make_unique<Channel>(inLoop, inSockfd))
    , m_localAddr(std::move(inLocalAddr))
    , m_peerAddr(std::move(inPeerAddr))
    , m_highWaterMark(kDefaultHighWaterMark)
{
    // Set callback functions for the channel
    m_channel->setReadCallback(
        [this](Timestamp t) { handleRead(t); }
    );
    m_channel->setWriteCallback(
        [this]() { handleWrite(); }
    );
    m_channel->setCloseCallback(
        [this]() { handleClose(); }
    );
    m_channel->setErrorCallback(
        [this]() { handleError(); }
    );

    LOG_INFO("TcpConnection::ctor[{}] at fd={}\n", m_name, inSockfd); // ctor = constructor
    m_socket->setKeepAlive(true);
}

void TcpConnection::send(std::string_view inMsg)
{
    if (m_state == State::Connected)
    {
        if (m_loop->isInLoopThread())
        {
            sendInLoop(inMsg.data(), inMsg.size());
        }
        else
        {
            m_loop->runInLoop([this, msg = std::string(inMsg)]() {
                sendInLoop(msg.data(), msg.size());
            });
        }
    }
}

void TcpConnection::sendInLoop(const void* inData, size_t inLen)
{
    ssize_t nwrote = 0;
    size_t remaining = inLen;
    bool faultError = false;

    // If the connection is already disconnected, give up writing
    if (m_state == State::Disconnected)
    {
        LOG_ERROR("disconnected, give up writing!");
        return;
    }

    // First write attempt if the channel is not writing and output buffer is empty
    if (!m_channel->isWriting() && m_outputBuffer.readableBytes() == 0)
    {
        nwrote = ::write(m_channel->getFd(), inData, inLen);
        if (nwrote >= 0)
        {
            remaining = inLen - nwrote;
            if (remaining == 0 && m_writeCompleteCallback)
            {
                m_loop->queueInLoop([this, self = shared_from_this()]() {
                    m_writeCompleteCallback(self);
                });
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    // If there's remaining data to be sent, append to buffer and enable writing
    if (!faultError && remaining > 0)
    {
        size_t oldLen = m_outputBuffer.readableBytes();
        if (oldLen + remaining >= m_highWaterMark
            && oldLen < m_highWaterMark
            && m_highWaterMarkCallback)
        {
            m_loop->queueInLoop([this, self = shared_from_this(), len = oldLen + remaining]() {
                m_highWaterMarkCallback(self, len);
            });
        }
        m_outputBuffer.append(static_cast<const char*>(inData) + nwrote, remaining);
        if (!m_channel->isWriting())
        {
            m_channel->enableWriting();
        }
    }
}

void TcpConnection::shutdown()
{
    if (m_state == State::Connected)
    {
        setState(State::Disconnecting);
        m_loop->runInLoop([this]() { shutdownInLoop(); });
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!m_channel->isWriting())
    {
        m_socket->shutdownWrite();
    }
}

void TcpConnection::connectEstablished()
{
    setState(State::Connected);
    m_channel->tie(shared_from_this());
    m_channel->enableReading();

    if (m_connectionCallback)
    {
        m_connectionCallback(shared_from_this());
    }
}

void TcpConnection::connectDestroyed()
{
    if (m_state == State::Connected)
    {
        setState(State::Disconnected);
        m_channel->disableAll();
        if (m_connectionCallback)
        {
            m_connectionCallback(shared_from_this());
        }
    }
    m_channel->remove();
}

void TcpConnection::handleRead(Timestamp inReceiveTime)
{
    int savedErrno = 0;
    ssize_t n = m_inputBuffer.readFd(m_channel->getFd(), &savedErrno);
    
    if (n > 0)
    {
        if (m_messageCallback)
        {
            m_messageCallback(shared_from_this(), &m_inputBuffer, inReceiveTime);
        }
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (m_channel->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = m_outputBuffer.writeFd(m_channel->getFd(), &savedErrno);
        
        if (n > 0)
        {
            m_outputBuffer.retrieve(n);
            if (m_outputBuffer.readableBytes() == 0)
            {
                m_channel->disableWriting();
                if (m_writeCompleteCallback)
                {
                    m_loop->queueInLoop([this, self = shared_from_this()]() {
                        m_writeCompleteCallback(self);
                    });
                }
                if (m_state == State::Disconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd={} is down, no more writing\n", m_channel->getFd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd={} state={}\n", m_channel->getFd(), static_cast<int>(m_state.load()));
    setState(State::Disconnected);
    m_channel->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    if (m_connectionCallback)
    {
        m_connectionCallback(connPtr);
    }
    if (m_closeCallback)
    {
        m_closeCallback(connPtr);
    }
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    
    if (::getsockopt(m_channel->getFd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:{} - SO_ERROR:{}\n", m_name, err);
}