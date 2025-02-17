#include "Thread.h"
#include <semaphore.h>
#include <syscall.h>
#include <cstdio>
#include <unistd.h>

namespace muduoModernCpp
{       
    // Static counter for tracking the total number of threads created
    std::atomic<int32_t> Thread::s_numCreated{0};

    /**
     * @brief Constructs a new Thread object
     * 
     * Initializes a new thread with the given function, name, and destruction behavior.
     * The thread is not started immediately after construction.
     * 
     * @param func The function to be executed in the thread
     * @param name The name of the thread (defaults to "Thread")
     * @param joinOnDestroy Whether to join or detach the thread in destructor (defaults to true)
     */
    Thread::Thread(ThreadFunc func, const std::string& name, bool joinOnDestroy)
        : m_func(std::move(func))
        , m_started(false)
        , m_joined(false)
        , m_joinOnDestroy(joinOnDestroy)
        , m_name(name)
        , m_tid(0)
    {
        setDefaultName();
    }

    /**
     * @brief Sets a default name for the thread if none was provided
     * 
     * Generates a name in the format "Thread<number>" where <number> is
     * an incrementing counter of created threads. This ensures each thread
     * has a unique default name.
     */
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

    /**
     * @brief Destructor for the Thread class
     * 
     * Handles cleanup of thread resources based on the joinOnDestroy setting.
     * Only performs cleanup if the thread was started but not already joined.
     * 
     * If the thread was started but not joined:
     * - If joinOnDestroy is true: joins the thread
     * - If joinOnDestroy is false: detaches the thread
     * 
     * @param joinOnDestroy Whether to join or detach the thread in destructor (defaults to true)
     */
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

    /**
     * @brief Starts the thread execution
     * 
     * Creates and starts the thread with the function provided in the constructor.
     * Uses a semaphore for synchronization to ensure the thread is fully initialized
     * before returning. This synchronization is necessary because thread scheduling
     * is non-deterministic.
     * 
     * The sequence is:
     * 1. Create a semaphore for synchronization
     * 2. Start the thread and assign thread ID
     * 3. Signal completion of thread initialization
     * 4. Execute the user's function
     * 5. Main thread waits for initialization before returning
     */
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

    /**
     * @brief Joins the thread if it was started
     * 
     * Blocks until the thread completes its execution.
     * Only attempts to join if the thread was actually started.
     * Sets m_joined to true after successful join to prevent
     * double-joining in destructor.
     */
    void Thread::join()
    {
        if (m_started)
        {
            m_threadPtr->join();
            m_joined = true;
        }
    }

} // namespace muduoModernCpp
