#pragma once
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

/**
 * @brief Handles new connection requests in the TCP server
 */
class Acceptor : noncopyable
{
public:
    /**
     * @brief Callback type for handling new connections
     */
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    /**
     * @brief Creates an acceptor with specified event loop and listening address
     * @param inLoop The main event loop for accepting new connections
     * @param inListenAddr The address to listen on
     * @param inReusePort Whether to enable SO_REUSEPORT option
     */
    Acceptor(EventLoop *inLoop, const InetAddress &inListenAddr, bool inReusePort);
    ~Acceptor();

    /**
     * @brief Sets the callback function for handling new connections
     * @param inCallback The callback function to be called when new connection arrives
     */
    void setNewConnectionCallback(const NewConnectionCallback &inCallback)
    {
        m_newConnectionCallback = inCallback;
    }

    /**
     * @brief Checks if the acceptor is currently listening
     * @return True if listening, false otherwise
     */
    bool listenning() const { return m_listenning; }

    /**
     * @brief Starts listening for new connections
     */
    void listen();
private:
    /**
     * @brief Handles the read event when new connection arrives
     */
    void handleRead();
    
    EventLoop *m_loop;         ///< The main reactor for accepting new connections
    Socket m_acceptSocket;     ///< Listening socket
    Channel m_acceptChannel;   ///< Channel for handling accept events
    NewConnectionCallback m_newConnectionCallback;  ///< Callback for processing new connections
    bool m_listenning;        ///< Whether the acceptor is listening
};
