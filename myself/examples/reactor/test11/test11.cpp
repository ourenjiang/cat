#include "myself/reactor/EventLoop.h"

#include <thread>
#include <unistd.h>
#include <iostream>

using namespace myself;
using namespace myself::reactor;

void threadFunc()
{
    printf("threadFunc(): pid = %d, tid = %d\n",
        getpid(), CurrentThread::tid());
    
    EventLoop loop;
    loop.loop();
}

int main()
{
    printf("main(): pid = %d, tid = %d\n",
        getpid(), CurrentThread::tid());

    EventLoop loop;

    std::thread thd(threadFunc);
    thd.detach();

    loop.loop();
}
