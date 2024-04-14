#include "myself/reactor/EventLoop.h"
#include "myself/reactor/Poller.h"
#include "myself/reactor/Channel.h"
#include "myself/reactor/TimerQueue.h"
#include "myself/base/SocketsOps.h"

#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/eventfd.h>

using namespace myself::reactor;

namespace
{
__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000;

int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  return evtfd;
}

class IgnoreSigPipe
{
 public:
  IgnoreSigPipe()
  {
    ::signal(SIGPIPE, SIG_IGN);
  }
};

IgnoreSigPipe initObj;
}

EventLoop::EventLoop()
    : quit_(false)
    , eventHandling_(false)
    , currentActiveChannel_(nullptr)
    , threadId_(CurrentThread::tid())
    , poller_(new Poller(this))
    , timerQueue_(new TimerQueue(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
    , callingPendingFunctors_(false)
{
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    ::close(wakeupFd_);
}

void EventLoop::loop()
{
    assert(isInLoopThread());
    quit_ = false;

    while(!quit_)
    {
        activeChannels_.clear();
        poller_->poll(kPollTimeMs, &activeChannels_);
        eventHandling_ = true;
        for(auto& channel : activeChannels_)
        {
            /**
             * 先标记当前正在执行事件分发的Channel
             * 用于检查‘事件回调’中移除Channel的情况
            */
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent();
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;
        doPendingFunctors();
    }
}

void EventLoop::quit()
{
  quit_ = true;
  if (!isInLoopThread())
  {
    wakeup();
  }
}

bool EventLoop::isInLoopThread() const
{
    return threadId_ == CurrentThread::tid();
}

void EventLoop::runInLoop(const Functor& cb)
{
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(cb);
  }
}

void EventLoop::queueInLoop(const Functor& cb)
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingFunctors_.push_back(cb);
  }

  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}


TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
  return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
  return timerQueue_->cancel(timerId);
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
    assert(n == sizeof(one));
}

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assert(isInLoopThread());
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assert(isInLoopThread());
    if (eventHandling_)
    {
        /**
         * 如果loop正在执行handleEvent
         * 那么有2种可能情况，其他都是异常：
         *  1, currentActiveChannel_ == channel  代表 removeChannel 的操作对象'可能是'自身，很好理解;
         *  2, std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end())
         *      即removeChannel的操作对象不在EventLoop的当前事件循环的激活事件列表之中
         * 
         * 如果【1】和【2】同时满足 => 绝不可能，很好理解。
         * 如果【1】满足 &【2】不满足 => 代表：操作对象'一定是'自身
         * 如果【1】不满足 & 【2】满足 => 代表：Channel就是一个没有激活的普通情况；
         * 
         * 断言所要排除的异常：activeChannels_中的元素意外地被泄漏了
        */
        assert(currentActiveChannel_ == channel ||
            std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}


void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
  }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
    {
        functors[i]();
    }
    callingPendingFunctors_ = false;
}