#pragma once

#include "noncopyable.h"
#include <functional>
#include <thread>
#include <memory>
#include <string>
#include <atomic>
#include <cstdint>

namespace muduoModernCpp
{
 class Thread : noncopyable
 { 
 public:
   using ThreadFunc = std::function<void()>;
   
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
   explicit Thread(ThreadFunc func, const std::string& name = std::string("Thread"), bool joinOnDestroy = true); // prevent implicit conversion
   
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
   ~Thread();
   
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
   void start();

   /**
    * @brief Joins the thread if it was started
    * 
    * Blocks until the thread completes its execution.
    * Only attempts to join if the thread was actually started.
    * Sets m_joined to true after successful join to prevent
    * double-joining in destructor.
    */
   void join();
   
   /**
    * @brief Returns whether the thread was started
    * 
    * @return true if the thread was started, false otherwise
    */
   bool started() const { return m_started; }

   /**
    * @brief Returns the thread ID of the thread
    * 
    * @return The thread ID of the thread
    */
   pid_t tid() const { return m_tid; }

   /**
    * @brief Returns the name of the thread
    * 
    * @return The name of the thread
    */
   const std::string& name() const { return m_name; }

   /**
    * @brief Returns the number of threads created
    * 
    * @return The number of threads created
    */
   static int32_t numCreated() { return s_numCreated; } 

 private:
   void runInThread();

   /**
    * @brief Sets a default name for the thread if none was provided
    * 
    * Generates a name in the format "Thread<number>" where <number> is
    * an incrementing counter of created threads. This ensures each thread
    * has a unique default name.
    */  
   void setDefaultName();

 private:
   ThreadFunc m_func;

   bool m_started;
   bool m_joined;
   bool m_joinOnDestroy;  // Controls whether to join or detach in destructor
   std::string m_name;
   pid_t m_tid;
   static std::atomic<int32_t> s_numCreated;

   std::shared_ptr<std::thread> m_threadPtr;
 };

}  // namespace muduoModernCpp
