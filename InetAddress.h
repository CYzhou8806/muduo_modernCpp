#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

/**
 * @brief Wrapper for socket address type
 * 
 * This class encapsulates IPv4 socket address operations and provides
 * a convenient interface for handling IP addresses and ports. It wraps
 * the sockaddr_in structure and provides methods for address conversion
 * and manipulation.
 */
class InetAddress
{
public:
    /**
     * @brief Constructs an InetAddress with specified port and IP
     * @param inPort The port number in host byte order
     * @param inIp The IP address string, defaults to localhost
     */
    explicit InetAddress(uint16_t inPort = 0, std::string inIp = "127.0.0.1");
    
    /**
     * @brief Constructs an InetAddress from an existing sockaddr_in
     * @param inAddr The sockaddr_in structure to wrap
     */
    explicit InetAddress(const sockaddr_in &inAddr)
        : m_addr(inAddr)
    {}

    /**
     * @brief Converts the IP address to string format
     * @return The IP address as a string
     */
    std::string toIp() const;

    /**
     * @brief Converts the IP address and port to string format
     * @return The IP:Port as a string
     */
    std::string toIpPort() const;

    /**
     * @brief Gets the port number
     * @return The port number in host byte order
     */
    uint16_t toPort() const;

    /**
     * @brief Gets the underlying sockaddr_in structure
     * @return Pointer to the internal sockaddr_in structure
     */
    const sockaddr_in* getSockAddr() const { return &m_addr; }

    /**
     * @brief Sets the internal sockaddr_in structure
     * @param inAddr The sockaddr_in structure to set
     */
    void setSockAddr(const sockaddr_in &inAddr) { m_addr = inAddr; }

private:
    sockaddr_in m_addr;  ///< The internal socket address structure
};
