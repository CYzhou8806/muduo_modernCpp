#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* inBaseLoop, const std::string& inNameArg)
    : m_baseLoop(inBaseLoop)
    , m_name(inNameArg)
    , m_started(false)
    , m_numThreads(0)
    , m_next(0)
{}

EventLoopThreadPool::~EventLoopThreadPool()
{}

void EventLoopThreadPool::start(const EventLoopThread::ThreadInitCallback &inCallback)
{
    m_started = true;

    if (m_numThreads == 0 && inCallback) // single-threaded mode
    {
        inCallback(m_baseLoop);
    }
    else // multi-threaded mode
    {
        // create sub-reactors
        for (int i = 0; i < m_numThreads; ++i)
        {
            char buf[m_name.size() + 32];
            snprintf(buf, sizeof buf, "%s%d", m_name.c_str(), i);
            EventLoopThread *thread = new EventLoopThread(inCallback, buf);
            m_threads.push_back(std::unique_ptr<EventLoopThread>(thread));
            m_loops.push_back(thread->startLoop()); // starts the thread, creates and binds an EventLoop in the new thread context
        }
    }
}


EventLoop* EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = m_baseLoop;

    /**
    * If working in multi-threaded mode, 
    * baseLoop_ defaults to assigning channels to subloops in a round-robin manner
    */
    if (!m_loops.empty()) // Get the next loop to process events through round-robin
    {
        loop = m_loops[m_next];
        ++m_next;
        if (m_next >= m_loops.size())
            m_next = 0;
    }
    return loop;    
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if (m_loops.empty())
    {
        return std::vector<EventLoop*>(1, m_baseLoop);
    }
    else
    {
        return m_loops;
    }
}
