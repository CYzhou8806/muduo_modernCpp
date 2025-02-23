#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// Prevent creating multiple EventLoops in one thread using thread_local
__thread EventLoop *t_loopInThisThread = nullptr;

// Default timeout for Poller IO multiplexing interface
const int kPollTimeMs = 10000;

/**
 * @brief Creates a non-blocking eventfd for wakeup mechanism
 * 
 * This is a helper function that creates an eventfd with non-blocking
 * and close-on-exec flags. It's used internally by EventLoop to
 * implement the wakeup mechanism between threads.
 * 
 * @return The created eventfd file descriptor
 */
static int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : m_looping(false)
    , m_quit(false)
    , m_callingPendingFunctors(false)
    , m_threadId(::syscall(SYS_gettid))
    , m_poller(Poller::newDefaultPoller(this))
    , m_wakeupFd(createEventfd())
    , m_wakeupChannel(new Channel(this, m_wakeupFd))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, m_threadId);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, m_threadId);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // Set the event type of wakeupfd and the callback operation after the event occurs
    m_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // Each EventLoop will listen for EPOLLIN read events on the wakeupChannel
    m_wakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
    m_wakeupChannel->disableAll();
    m_wakeupChannel->remove();
    ::close(m_wakeupFd);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    m_looping = true;
    m_quit = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!m_quit)
    {
        m_activeChannels.clear();
        // Monitor two types of fd: client fd and wakeup fd
        m_pollReturnTime = m_poller->poll(kPollTimeMs, &m_activeChannels);
        for (Channel *channel : m_activeChannels)
        {
            // Poller monitors which channels have events, reports to EventLoop, and notifies channels to handle corresponding events
            channel->handleEvent(m_pollReturnTime);
        }
        // Execute callback operations that need to be processed in the current EventLoop
        /**
         * IO thread mainLoop accept fd<=channel subloop
         * mainLoop pre-registers a callback cb (to be executed by subloop), after waking up subloop,
         * execute the method below to perform the cb operation previously registered by mainloop
         */ 
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    m_looping = false;
}

/**
 * Exit the event loop: 
 * 1. loop calls quit in its own thread 
 * 2. quit is called in a non-loop thread
 */ 
void EventLoop::quit()
{
    m_quit = true;

    // If quit is called in a non-loop thread, wake up the loop thread
    if (!isInLoopThread())  
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor inCallback)
{
    if (isInLoopThread()) // Execute cb in the current loop thread
    {
        inCallback();
    }
    else // Execute cb in a non-current loop thread, need to wake up the loop thread to execute cb
    {
        queueInLoop(inCallback);
    }
}

void EventLoop::queueInLoop(Functor inCallback)
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_pendingFunctors.emplace_back(inCallback);
    }

    // Wake up the loop thread that needs to execute the above callback operations
    // || m_callingPendingFunctors means: current loop is executing callbacks, but loop has new callbacks
    if (!isInLoopThread() || m_callingPendingFunctors) 
    {
        wakeup(); // Wake up the loop thread
    }
}

void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = read(m_wakeupFd, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
  }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(m_wakeupFd, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop methods => Poller methods
void EventLoop::updateChannel(Channel *inChannel)
{
    m_poller->updateChannel(inChannel);
}

void EventLoop::removeChannel(Channel *inChannel)
{
    m_poller->removeChannel(inChannel);
}

bool EventLoop::hasChannel(Channel *inChannel)
{
    return m_poller->hasChannel(inChannel);
}

void EventLoop::doPendingFunctors() // Execute callbacks
{
    std::vector<Functor> functors;
    m_callingPendingFunctors = true;

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        functors.swap(m_pendingFunctors); // swap instead of move, more efficient
    }

    for (const Functor &functor : functors)
    {
        functor(); // Execute callback operations that the current loop needs to perform
    }

    m_callingPendingFunctors = false;
}
