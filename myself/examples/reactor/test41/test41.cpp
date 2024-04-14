#include "myself/communicate/domain/DomainServer.h"
#include "myself/reactor/EventLoopThread.h"
#include "myself/reactor/EventLoop.h"

#include <iostream>
#include <stdlib.h>
#include <unistd.h>

using namespace std;
using namespace myself;

int main()
{
    reactor::EventLoopThread loopThread;
    string listenAddr("/var/tmp/domain_test_001");

    auto loopptr = loopThread.startLoop();
    domain::DomainServer server(loopptr, listenAddr);

    mutex mutex_;
    domain::ConnectionPtr connection_;

    {
        server.setConnectionCallback([&](const domain::ConnectionPtr& conn){
            printf("---connection: %s -> %s is %s\n",
                conn->localAddress().c_str(),
                conn->peerAddress().c_str(),
                conn->connected() ? "UP" : "DOWN");

            lock_guard<mutex> lock(mutex_);
            if(conn->connected())
            {
                connection_ = conn;
            }
            else
            {
                connection_.reset();
            }
        });
        server.setMessageCallback([](const domain::ConnectionPtr& conn, Buffer* buffer){
            std::string data(buffer->peek(), buffer->peek() + buffer->readableBytes());
            buffer->retrieveAll();
            printf("---message: %s\n", data.c_str());
            conn->send("---hello " + data + " world");
        });
        server.start();
    }

    // 进入业务线程范围
    std::string line;
    while (std::getline(std::cin, line))
    {
        if(line == "quit")
        {
            loopptr->quit();
            usleep(100);
            break;
        }

        lock_guard<mutex> lock(mutex_);
        if(connection_)
        {
            connection_->send(line);
        }
    }
}
