#pragma once
#include "myself/base/Thread.h"
#include "myself/base/Timestamp.h"
#include "myself/reactor/TimerId.h"
#include "myself/reactor/Callbacks.h"

#include <sys/types.h>
#include <memory>
#include <vector>
#include <mutex>
#include <functional>

namespace myself
{
namespace reactor
{

class Channel;
class Poller;
class TimerQueue;

class EventLoop
{
public:
    typedef std::vector<Channel*> ChannelList;
    typedef std::function<void()> Functor;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    bool isInLoopThread() const;

    void runInLoop(const Functor& cb);
    void queueInLoop(const Functor& cb);

    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    TimerId runAfter(double delay, const TimerCallback& cb);
    TimerId runEvery(double interval, const TimerCallback& cb);
    void cancel(TimerId timerId);

    void wakeup();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    void handleRead();  // waked up
    void doPendingFunctors();

    bool quit_; /* atomic */
    bool eventHandling_;
    Channel* currentActiveChannel_;
    const pid_t threadId_;
    const int kPollTimeMs = 10000;
    std::unique_ptr<Poller> poller_;
    ChannelList activeChannels_;
    std::mutex mutex_;
    std::vector<Functor> pendingFunctors_;
    bool callingPendingFunctors_;
    std::unique_ptr<TimerQueue> timerQueue_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
};

}//namespace reactor
}//namespace myself
