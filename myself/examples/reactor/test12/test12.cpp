#include "myself/reactor/EventLoop.h"

#include <thread>
#include <unistd.h>
#include <iostream>

using namespace myself;

reactor::EventLoop *g_loop;

void threadFunc()
{
    printf("threadFunc(): pid = %d, tid = %d\n",
        getpid(), CurrentThread::tid());
    
    g_loop->loop();
}

int main()
{
    printf("main(): pid = %d, tid = %d\n",
        getpid(), CurrentThread::tid());

    reactor::EventLoop loop;
    g_loop = &loop;

    std::thread thd(threadFunc);
    thd.detach();
}
