#include "myself/reactor/EventLoopThread.h"
#include "myself/reactor/EventLoop.h"

#include <assert.h>

using namespace myself::reactor;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
  : loop_(NULL)
  , exiting_(false)
  , callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  loop_->quit();
  thread_.join();
}

EventLoop* EventLoopThread::startLoop()
{
//   assert(!thread_.started());
//   thread_.start();
    assert(!loop_);
    thread_ = std::thread(&EventLoopThread::threadFunc, this);

    {
      std::unique_lock<std::mutex> lock(mutex_);
      while (loop_ == NULL)
      {
        cond_.wait(lock);
      }
    }
    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    if (callback_)
    {
        callback_(&loop);
    }

    {
        // MutexLockGuard lock(mutex_);
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_all();
    }

    loop.loop();
}
