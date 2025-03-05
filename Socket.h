#pragma once

#include "noncopyable.h"

class InetAddress;

/**
 * @brief RAII wrapper for socket file descriptor
 * 
 * This class provides a C++ interface for socket operations and ensures
 * proper resource management through RAII. The socket is automatically
 * closed when the object is destroyed.
 */
class Socket : noncopyable
{
public:
    /**
     * @brief Constructs a Socket object with an existing socket file descriptor
     * @param inSockfd The socket file descriptor to wrap
     */
    explicit Socket(int inSockfd)
        : m_sockfd(inSockfd)
    {}

    /**
     * @brief Destroys the Socket object and closes the underlying file descriptor
     */
    ~Socket();

    /**
     * @brief Gets the underlying socket file descriptor
     * @return The socket file descriptor
     */
    int fd() const { return m_sockfd; }

    /**
     * @brief Binds the socket to a local address
     * @param inLocaladdr The local address to bind to
     */
    void bindAddress(const InetAddress &inLocaladdr);

    /**
     * @brief Marks the socket as a passive socket for accepting connections
     */
    void listen();

    /**
     * @brief Accepts a new connection on the listening socket
     * @param inPeeraddr The address of the connecting peer
     * @return The new connection's socket file descriptor
     */
    int accept(InetAddress *inPeeraddr);

    /**
     * @brief Disables further send operations on the socket
     */
    void shutdownWrite();

    /**
     * @brief Sets TCP_NODELAY option (disables Nagle's algorithm)
     * @param inOn True to enable the option, false to disable
     */
    void setTcpNoDelay(bool inOn);

    /**
     * @brief Sets SO_REUSEADDR option
     * @param inOn True to enable the option, false to disable
     */
    void setReuseAddr(bool inOn);

    /**
     * @brief Sets SO_REUSEPORT option
     * @param inOn True to enable the option, false to disable
     */
    void setReusePort(bool inOn);

    /**
     * @brief Sets SO_KEEPALIVE option
     * @param inOn True to enable the option, false to disable
     */
    void setKeepAlive(bool inOn);

private:
    const int m_sockfd;  ///< The underlying socket file descriptor
};