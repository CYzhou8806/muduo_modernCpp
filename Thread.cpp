#include "Thread.h"
#include <semaphore.h>
#include <syscall.h>
#include <cstdio>
#include <unistd.h>

namespace muduoModernCpp
{       
    // Static counter for tracking the total number of threads created
    std::atomic<int32_t> Thread::s_numCreated{0};

    Thread::Thread(ThreadFunc inFunc, const std::string& inName, bool inJoinOnDestroy)
        : m_func(std::move(inFunc))
        , m_started(false)
        , m_joined(false)
        , m_joinOnDestroy(inJoinOnDestroy)
        , m_name(inName)
        , m_tid(0)
    {
        setDefaultName();
    }

    void Thread::setDefaultName()
    {
        int num = ++s_numCreated;
        if (m_name.empty())
        {
            char buf[32];
            snprintf(buf, sizeof buf, "Thread%d", num);
            m_name = buf;
        }
    }

    Thread::~Thread()
    {
        if (m_started && !m_joined)
        {
            if (m_joinOnDestroy)
            {
                m_threadPtr->join();
            }
            else
            {
                m_threadPtr->detach();
            }
        }
    }

    void Thread::start()
    {
        if (!m_started)
        {
            m_started = true;

            sem_t sem;
            sem_init(&sem, 0, 0);

            // Create and start the thread
            m_threadPtr = std::make_shared<std::thread>([&]() {
                m_tid = static_cast<pid_t>(::syscall(SYS_gettid));
                sem_post(&sem);
                
                if (m_func)
                {
                    m_func();
                }
            });

            // Wait for thread to start and get its ID
            sem_wait(&sem);
            sem_destroy(&sem);
        }
    }

    void Thread::join()
    {
        if (m_started)
        {
            m_threadPtr->join();
            m_joined = true;
        }
    }

} // namespace muduoModernCpp
