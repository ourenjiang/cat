#include "myself/reactor/Poller.h"
#include "myself/reactor/Channel.h"
#include "myself/reactor/EventLoop.h"

#include <assert.h>
#include <string.h>

using namespace myself::reactor;

Poller::Poller(EventLoop* loop)
    : loop_(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)
{
}

Poller::~Poller()
{
}

void Poller::poll(int timeoutMs, ChannelList* activeChannels)
{
    int numEvents = ::epoll_wait(epollfd_,
                               events_.data(),
                               static_cast<int>(events_.size()),
                               timeoutMs);
    if (numEvents > 0)
    {
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size())
        {
        events_.resize(events_.size()*2);
        }
    }
    else if (numEvents == 0)
    {
    }
    else
    {
    }
}

void Poller::updateChannel(Channel* channel)
{
    assert(loop_->isInLoopThread());

    const int index = channel->index();
    if (index == kNew || index == kDeleted)
    {
        /**
         * 总结：如果一个Channel是全新的或是曾经注册过但现在已经是删除状态，
         * 那么调用updateChannel都将使状态被设置为'已添加'，并将其注册到epoll监听集之中;
         * 
         * 对于那些被设置为'全新'状态的描述符，不能在channels_列表中找到;
         * 对于那些被设置为'已删除'状态的描述符，仍能在channels_列表中找到;
         * 
         * 如何理解 kDeleted 状态时，执行updateChannel却要将其再将添加到监听集之中吗，不应该是删除吗;
         * 回答：kNew  kAdded  kDeleted 这3种状态，代表fd在Poller（中间层）保存的状态;
         * kNew      代表从未记录过，是全新的；
         * kAdded    代表已记录，并且进一步注册到epoll监听集中
         * kDeleted  代表曾经记录过，但中途从epoll监听集中删除，但并未从Poller记录中删除（留待后续重返epoll监听集);
        */

        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else // index == kDeleted
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else// index == kAdded
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);

        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Poller::removeChannel(Channel* channel)
{
    /**
     * 对于那些将来绝不可能重返epoll监听集的Poller记录，调用removeChannel直接删除!!!
    */
    assert(loop_->isInLoopThread());

    int fd = channel->fd();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());

    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);

    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void Poller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    assert(numEvents <= events_.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);

        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);

        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void Poller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
    }
}
