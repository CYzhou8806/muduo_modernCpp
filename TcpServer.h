#pragma once

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>


/**
 * @brief TCP server class for users to write server programs using muduo
 * 
 * This class manages TCP connections and provides a high-level interface
 * for building TCP servers. It supports multi-threading through a thread pool
 * where each thread runs its own event loop.
 */
class TcpServer final : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    enum class Option
    {
        NoReusePort,
        ReusePort,
    };

    /**
     * @brief Constructs a TCP server
     * @param inLoop The main event loop
     * @param inListenAddr The address to listen on
     * @param inName Server name
     * @param inOption Port reuse option
     * @throws std::runtime_error if inLoop is null
     */
    TcpServer(EventLoop *inLoop,
              const InetAddress &inListenAddr,
              std::string inName,
              Option inOption = Option::NoReusePort);

    // Use default destructor and explicitly state move semantics
    ~TcpServer();

    // Callback setters using fluent interface
    TcpServer& setThreadInitCallback(ThreadInitCallback inCb) noexcept 
    { 
        m_threadInitCallback = std::move(inCb); 
        return *this;
    }
    
    TcpServer& setConnectionCallback(ConnectionCallback inCb) noexcept 
    { 
        m_connectionCallback = std::move(inCb); 
        return *this;
    }
    
    TcpServer& setMessageCallback(MessageCallback inCb) noexcept 
    { 
        m_messageCallback = std::move(inCb); 
        return *this;
    }
    
    TcpServer& setWriteCompleteCallback(WriteCompleteCallback inCb) noexcept 
    { 
        m_writeCompleteCallback = std::move(inCb); 
        return *this;
    }

    // Getters
    [[nodiscard]] const std::string& getIpPort() const noexcept { return m_ipPort; }
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }
    [[nodiscard]] EventLoop* getLoop() const noexcept { return m_loop; }

    /**
     * @brief Set the number of threads in the thread pool
     * @param inNumThreads Number of threads to use
     * @throws std::invalid_argument if inNumThreads is negative
     */
    void setThreadNum(int inNumThreads);

    /**
     * @brief Start the server
     * @note Thread-safe and idempotent
     */
    void start();

private:
    void newConnection(int inSockfd, const InetAddress &inPeerAddr);
    void removeConnection(const TcpConnectionPtr &inConn);
    void removeConnectionInLoop(const TcpConnectionPtr &inConn);

    // Essential server components
    EventLoop* const m_loop;  // baseLoop defined by user
    const std::string m_ipPort;
    const std::string m_name;
    std::unique_ptr<Acceptor> m_acceptor;  // runs in mainLoop, monitors new connection events
    std::shared_ptr<EventLoopThreadPool> m_threadPool;  // one loop per thread

    // Callback handlers
    ConnectionCallback m_connectionCallback;      // callback for new connections
    MessageCallback m_messageCallback;            // callback for read/write messages
    WriteCompleteCallback m_writeCompleteCallback;// callback after message sending completes
    ThreadInitCallback m_threadInitCallback;      // callback for loop thread initialization

    // Server state
    std::atomic<bool> m_started{false};
    std::atomic<int> m_nextConnId{1};
    ConnectionMap m_connections;  // stores all connections
};
