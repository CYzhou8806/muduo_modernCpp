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
   
   explicit Thread(ThreadFunc func, const std::string& name = std::string("Thread"), bool joinOnDestroy = true); // prevent implicit conversion
   ~Thread();
   
   void start();
   void join();
   bool started() const { return m_started; }
   pid_t tid() const { return m_tid; }
   const std::string& name() const { return m_name; }
   static int32_t numCreated() { return s_numCreated; } 

 private:
   void runInThread();
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
}
