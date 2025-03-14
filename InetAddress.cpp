#include "InetAddress.h"

#include <strings.h>
#include <string.h>

InetAddress::InetAddress(uint16_t inPort, std::string inIp)
{
    bzero(&m_addr, sizeof m_addr);
    m_addr.sin_family = AF_INET; // using ipv4 protocol
    m_addr.sin_port = htons(inPort);
    m_addr.sin_addr.s_addr = inet_addr(inIp.c_str());
}

std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof buf);
    return buf;
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(m_addr.sin_port);
    sprintf(buf+end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(m_addr.sin_port);
}
