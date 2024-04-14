#include "myself/reactor/EventLoop.h"
#include "myself/reactor/Channel.h"


#include <sys/timerfd.h>
#include <iostream>
#include <string.h>
#include <unistd.h>

using namespace myself;

reactor::EventLoop *g_loop;

void onTimeout()
{
    printf("onTimeout\n");
}

int main()
{
    reactor::EventLoop loop;
    g_loop = &loop;

    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    reactor::Channel channel(&loop, timerfd);
    channel.setReadCallback(onTimeout);
    channel.enableReading();

    struct itimerspec howlong;
    bzero(&howlong, sizeof(howlong));
    howlong.it_value.tv_sec = 5;
    ::timerfd_settime(timerfd, 0, &howlong, nullptr);

    loop.loop();
    ::close(timerfd);
}
