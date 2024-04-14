#include "myself/reactor/Channel.h"
#include "myself/reactor/EventLoop.h"

#include <assert.h>

using namespace myself::reactor;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(kNonEvent)
    , revents_(kNonEvent)
    , index_(-1)
    , tied_(false)
    , eventHandling_(false)
{
}

Channel::~Channel()
{
    // 确保：在释放时，IO线程一定不是执行其事件分发；
    assert(!eventHandling_);
}

void Channel::handleEvent()
{
    std::shared_ptr<void> guard;//为什么guard需要提前定义在这里呢？是不是只是为了逻辑清晰？可以直接在内层条件判断中直接使用临时对象吗？
    if(tied_)
    {
        /**
         * 如果提升成功，TcpConnection的引用计数(Channel的宿主)加1，防止在handleEventWithGuard()中降为0
        */
        guard = tie_.lock();
        if(guard)
        {
            /**
             * 如果客户端断开连接，TcpConnection的引用计数将在handleEventWithGuard()中减小1，
            */
            handleEventWithGuard();
        }
        else
        {
            /**
             * 进入这里，代表Channel的生命周期长于TcpConnection(宿主)，这怎么可能呢？
             * 解释：TcpConnection只是一个特殊，自然也不可能进入该位置。
             * 扩大范围：如果Channel是从外部引入其宿主，其生命周期不限于宿主，只是其分发的事件回调存在于宿主而已，那就有可能进入这个位置了
             * 进入这里，代表宿主没了，自然Channel事件分发所需要执行的事件回调也已经是不存在了。
            */
        }
    }
    else
    {
        handleEventWithGuard();
    }
}

void Channel::tie(const std::shared_ptr<void>& owner)
{
    tie_ = owner;
    tied_ = true;
}

void Channel::remove()
{
    assert(isNoneEvent());
    loop_->removeChannel(this);
}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::handleEventWithGuard()
{
    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (readCallback_) readCallback_();
    }
    if (revents_ & POLLOUT)
    {
        if (writeCallback_) writeCallback_();
    }
}
