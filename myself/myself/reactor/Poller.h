#pragma once
#include <vector>
#include <map>
#include <sys/epoll.h>

// struct epoll_event; 这里与源码所描述的不一致，vector<struct epoll_event> 编译器提示不完整类型，估计是编译版本问题

namespace myself
{
namespace reactor
{

class EventLoop;
class Channel;

class Poller
{
public:
    typedef std::vector<Channel*> ChannelList;
    typedef std::vector<struct epoll_event> EventList;
    typedef std::map<int, Channel*> ChannelMap;// < fd, Channel>

    Poller(EventLoop* loop);
    ~Poller();
    void poll(int timeoutMs, ChannelList* activeChannels);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    const int kNew = -1;
    const int kAdded = 1;
    const int kDeleted = 2;

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    EventLoop* loop_;
    const int kInitEventListSize = 16;
    int epollfd_;
    EventList events_;
    ChannelMap channels_;
};

}//namespace reactor
}//namespace myself
