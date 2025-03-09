#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/**
 * @brief TCP connection class that handles individual connections
 * 
 * Flow: TcpServer => Acceptor => New connection (connfd from accept)
 * => TcpConnection sets callbacks => Channel => Poller => Channel callbacks
 */
class TcpConnection final : noncopyable,
                            public std::enable_shared_from_this<TcpConnection> {
public:
    /**
     * @brief Constructs a TCP connection
     * @param inLoop Event loop that manages this connection
     * @param inName Connection identifier
     * @param inSockfd Socket file descriptor
     * @param inLocalAddr Local address
     * @param inPeerAddr Peer address
     */
    TcpConnection(EventLoop *inLoop,
                  std::string inName,
                  int inSockfd,
                  InetAddress inLocalAddr,
                  InetAddress inPeerAddr);
    
    ~TcpConnection() = default;

    // Getters
    [[nodiscard]] EventLoop* getLoop() const noexcept { return m_loop; }
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] const InetAddress& getLocalAddress() const noexcept { return m_localAddr; }
    [[nodiscard]] const InetAddress& getPeerAddress() const noexcept { return m_peerAddr; }
    [[nodiscard]] bool isConnected() const noexcept { return m_state == State::Connected; }

    // Callback setters
    TcpConnection& setConnectionCallback(ConnectionCallback inCb) noexcept
    { m_connectionCallback = std::move(inCb); return *this; }

    TcpConnection& setMessageCallback(MessageCallback inCb) noexcept
    { m_messageCallback = std::move(inCb); return *this; }

    TcpConnection& setWriteCompleteCallback(WriteCompleteCallback inCb) noexcept
    { m_writeCompleteCallback = std::move(inCb); return *this; }

    TcpConnection& setHighWaterMarkCallback(HighWaterMarkCallback inCb, size_t inHighWaterMark) noexcept
    { 
        m_highWaterMarkCallback = std::move(inCb); 
        m_highWaterMark = inHighWaterMark; 
        return *this; 
    }

    TcpConnection& setCloseCallback(CloseCallback inCb) noexcept
    { m_closeCallback = std::move(inCb); return *this; }

    /**
     * @brief Establish the connection
     * @details Called when the connection is successfully established
     */
    void connectEstablished();

    /**
     * @brief Destroy the connection
     * @details Called when the connection is being torn down
     */
    void connectDestroyed();

    /**
     * @brief Send a message through the connection
     * @param inMsg Message to send
     */
    void send(std::string_view inMsg);

    /**
     * @brief Initiate connection shutdown
     * @details Gracefully closes the write end of the connection
     */
    void shutdown();

private:
    enum class State 
    {
        Disconnected,
        Connecting,
        Connected,
        Disconnecting
    };

    void setState(State inState) noexcept { m_state = inState; }

    /**
     * @brief Handle read events
     * @param inReceiveTime Timestamp when the read event occurred
     */
    void handleRead(Timestamp inReceiveTime);

    /**
     * @brief Handle write events
     */
    void handleWrite();

    /**
     * @brief Handle connection close events
     */
    void handleClose();

    /**
     * @brief Handle error events
     */
    void handleError();

    /**
     * @brief Send message in the event loop
     * @param inMessage Pointer to the message data
     * @param inLen Length of the message in bytes
     */
    void sendInLoop(const void* inMessage, size_t inLen);

    /**
     * @brief Perform shutdown in the event loop
     */
    void shutdownInLoop();

private: // attributes
    // Essential components
    EventLoop* const m_loop;  // subLoop that manages this connection
    const std::string m_name;
    std::atomic<State> m_state{State::Disconnected};
    std::atomic<bool> m_reading{false};

    // Socket management
    std::unique_ptr<Socket> m_socket;   // RAII handle for socket fd
    std::unique_ptr<Channel> m_channel; // Channel for event handling

    // Connection addresses
    const InetAddress m_localAddr;
    const InetAddress m_peerAddr;

    // Callback handlers
    ConnectionCallback m_connectionCallback;       // New connection callback
    MessageCallback m_messageCallback;            // Read/write message callback
    WriteCompleteCallback m_writeCompleteCallback;// Write completion callback
    HighWaterMarkCallback m_highWaterMarkCallback;// High water mark callback
    CloseCallback m_closeCallback;               // Connection close callback
    
    // Configuration
    size_t m_highWaterMark{0};

    // I/O buffers
    Buffer m_inputBuffer;   // Receive buffer
    Buffer m_outputBuffer;  // Send buffer
};
