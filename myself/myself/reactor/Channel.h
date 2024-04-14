#pragma once
#include <memory>
#include <functional>
#include <poll.h>

namespace myself
{
namespace reactor
{

class EventLoop;

class Channel
{
public:
    typedef std::function<void()> EventCallback;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    void handleEvent();
    void setReadCallback(const EventCallback& cb){ readCallback_ = cb; }
    void setWriteCallback(const EventCallback& cb){ writeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb){ errorCallback_ = cb; }
    void setCloseCallback(const EventCallback& cb){ closeCallback_ = cb; }

    /// Tie this channel to the owner object managed by shared_ptr,
    /// prevent the owner object being destroyed in handleEvent.
    /// 只有被shared_ptr管理的Channel宿主，才有必要执行tie操作。
    /// 防止在执行handleEvent的过程中，引用计数降为0。
    /// 对于在事件回调内部释放了Channel的宿主，比如TcpConnection在用户的事件回调中执行断开操作，
    /// TcpConnection的shared_ptr引用计数可能减小为0，从而提前于handleEvent调用栈而释放，这使得Channel变成空悬指针。
    /// 通过tie操作，在执行具体的事件分发操作前，先判断TcpConnection是否已经被销毁。
    void tie(const std::shared_ptr<void>& owner);

    int fd() { return fd_; }
    int events() { return events_; }
    void set_revents(int revt) { revents_ = revt; }
    bool isNoneEvent() const { return events_ == kNonEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    void enableReading() { events_ |= kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNonEvent; update(); }

    // for Poller
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop* ownerLoop() { return loop_; }
    void remove();

private:
    void update();
    void handleEventWithGuard();

    const int kNonEvent = 0;
    const int kReadEvent = POLLIN;
    const int kWriteEvent = POLLOUT;

    EventLoop *loop_;
    const int fd_;
    int events_;//关注的事件，由用户设置
    int revents_;//当前活动的事件，由Poller设置
    int index_; // used by Poller.

    bool tied_;
    std::weak_ptr<void> tie_;
    bool eventHandling_;

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
};

}//namespace reactor
}//namespace myself
