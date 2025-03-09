#include "Buffer.h"

#include <array>
#include <system_error>
#include <sys/uio.h>
#include <unistd.h>

ssize_t Buffer::readFd(int inFd, int* inSaveErrno)
{
    static constexpr size_t kExtraBufSize = 65536;  // 64KB stack buffer
    std::array<char, kExtraBufSize> extraBuf{};
    
    std::array<struct iovec, 2> vec{{
        { begin() + m_writerIndex, writableBytes() },  // buffer space
        { extraBuf.data(), extraBuf.size() }         // stack space
    }};
    
    const auto writable = writableBytes();
    const int iovcnt = (writable < extraBuf.size()) ? 2 : 1;
    const auto n = ::readv(inFd, vec.data(), iovcnt);
    if (n < 0)
    {
        *inSaveErrno = errno;
        return n;
    }
    else if (n <= static_cast<ssize_t>(writable))
    {
        m_writerIndex += n;
    }
    else
    {
        m_writerIndex = m_buffer.size();
        append(extraBuf.data(), n - writable);
    }
    
    return n;
}

/**
 * @brief Write buffer contents to a file descriptor
 * @details Performs a single write operation for all readable data
 */
ssize_t Buffer::writeFd(int inFd, int* inSaveErrno)
{
    const auto readable = readableBytes();
    if (readable == 0) { return 0; }
    
    if (const auto n = ::write(inFd, peek(), readable); n < 0)
    {
        *inSaveErrno = errno;
        return n;
    }
    else
    {
        return n;
    }
}