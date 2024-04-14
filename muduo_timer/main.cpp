#include "EventLoop.h"
#include <iostream>

using namespace std;
using namespace muduo;
using namespace muduo::net;

int main()
{
    EventLoop loop;
    loop.runEvery(1.5, [&]{
        cout << "timeout" << endl;
    });
    loop.loop();
}
