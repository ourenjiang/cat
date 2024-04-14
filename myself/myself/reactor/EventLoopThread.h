#pragma once
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace myself
{
namespace reactor
{

class EventLoop;

class EventLoopThread
{
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
  ~EventLoopThread();
  EventLoop* startLoop();

 private:
  void threadFunc();

  EventLoop* loop_;
  bool exiting_;
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback callback_;
};

}//namespace reactor
}//namespace myself
