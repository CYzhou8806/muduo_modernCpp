#include "EventLoopThread.h"
#include "EventLoop.h"
#include <syscall.h>

namespace muduoModernCpp
{       
EventLoopThread::EventLoopThread(const ThreadInitCallback &inCallback, 
        const std::string &inName)
        : m_loop(nullptr)
        , m_exiting(false)
        , m_thread(std::bind(&EventLoopThread::threadFunc, this), inName)
        , m_mutex()
        , m_cond()
        , m_callback(inCallback)
{
}

EventLoopThread::~EventLoopThread()
{
    m_exiting = true;
    if (m_loop != nullptr)
    {
        m_loop->quit();
        m_thread.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    m_thread.start(); // create and start the new thread,  and run the threadFunc()

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while ( m_loop == nullptr )
        {
            m_cond.wait(lock);
        }

        // here, the loop object from EventLoopThread::threadFunc() is ready
        loop = m_loop; // use raw pointer as observer
    }
    return loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop; // one EventLoop per thread

    if (m_callback)
    {
        m_callback(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_loop = &loop;
        m_cond.notify_one();
    }

    loop.loop(); // block in the EventLoop loop  => Poller.poll

    // after m_quit in loop set as true, exit the while loop from loop.loop() 
    // process will come to here and destroy the above EventLoop object
    std::unique_lock<std::mutex> lock(m_mutex);
    m_loop = nullptr;
}

} // namespace muduoModernCpp
