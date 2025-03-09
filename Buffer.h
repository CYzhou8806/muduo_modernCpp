#pragma once

#include <vector>
#include <string>
#include <algorithm>

/**
 * @brief Network library underlying buffer implementation
 * @details Provides efficient buffer management for network I/O operations
 */
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    /**
     * @brief Constructs a buffer with specified initial size
     * @param initialSize Initial size of the buffer (excluding prepend space)
     */
    explicit Buffer(size_t inInitialSize = kInitialSize)
        : m_buffer(kCheapPrepend + inInitialSize)
        , m_readerIndex(kCheapPrepend)
        , m_writerIndex(kCheapPrepend)
    {}

    /**
     * @brief Get the number of readable bytes in buffer
     * @return Number of bytes available for reading
     */
    size_t readableBytes() const 
    {
        return m_writerIndex - m_readerIndex;
    }

    /**
     * @brief Get the number of writable bytes in buffer
     * @return Number of bytes available for writing
     */
    size_t writableBytes() const
    {
        return m_buffer.size() - m_writerIndex;
    }

    /**
     * @brief Get the number of prependable bytes
     * @return Number of bytes available for prepending
     */
    size_t prependableBytes() const
    {
        return m_readerIndex;
    }

    /**
     * @brief Get pointer to the beginning of readable data
     * @return Const pointer to the first readable byte
     */
    const char* peek() const
    {
        return begin() + m_readerIndex;
    }

    /**
     * @brief Retrieve data from buffer
     * @param len Number of bytes to retrieve
     * @details If len is less than readable bytes, only retrieve len bytes;
     *          otherwise retrieve all readable data
     */
    void retrieve(size_t inLen)
    {
        if (inLen < readableBytes())
        {
            m_readerIndex += inLen;
        }
        else   // inLen == readableBytes()
        {
            retrieveAll();
        }
    }

    /**
     * @brief Reset buffer to initial state
     * @details Moves both read and write indices to kCheapPrepend position
     */
    void retrieveAll()
    {
        m_readerIndex = m_writerIndex = kCheapPrepend;
    }

    /**
     * @brief Retrieve all readable data as string
     * @return String containing all readable data
     */
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    /**
     * @brief Retrieve specified length of data as string
     * @param len Number of bytes to retrieve
     * @return String containing retrieved data
     */
    std::string retrieveAsString(size_t inLen)
    {
        std::string result(peek(), inLen);
        retrieve(inLen);
        return result;
    }

    /**
     * @brief Ensure buffer has enough space for writing
     * @param len Required space in bytes
     * @details Expands buffer if necessary
     */
    void ensureWriteableBytes(size_t inLen)
    {
        if (writableBytes() < inLen)
        {
            makeSpace(inLen);
        }
    }

    /**
     * @brief Append data to buffer
     * @param data Pointer to data to append
     * @param len Length of data in bytes
     */
    void append(const char *inData, size_t inLen)
    {
        ensureWriteableBytes(inLen);
        std::copy(inData, inData+inLen, beginWrite());
        m_writerIndex += inLen;
    }

    char* beginWrite()
    {
        return begin() + m_writerIndex;
    }

    const char* beginWrite() const
    {
        return begin() + m_writerIndex;
    }

    /**
     * @brief Read data from file descriptor
     * @param fd File descriptor to read from
     * @param saveErrno Pointer to store error number
     * @return Number of bytes read, -1 on error
     */
    ssize_t readFd(int inFd, int* inSaveErrno);

    /**
     * @brief Write data to file descriptor
     * @param fd File descriptor to write to
     * @param saveErrno Pointer to store error number
     * @return Number of bytes written, -1 on error
     */
    ssize_t writeFd(int inFd, int* inSaveErrno);
private:
    char* begin()
    {
        return &*m_buffer.begin();
    }
    const char* begin() const
    {
        return &*m_buffer.begin();
    }
    void makeSpace(size_t inLen)
    {
        if (writableBytes() + prependableBytes() < inLen + kCheapPrepend)
        {
            m_buffer.resize(m_writerIndex + inLen);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + m_readerIndex, 
                    begin() + m_writerIndex,
                    begin() + kCheapPrepend);
            m_readerIndex = kCheapPrepend;
            m_writerIndex = m_readerIndex + readable;
        }
    }

    std::vector<char> m_buffer;
    size_t m_readerIndex;
    size_t m_writerIndex;
};
