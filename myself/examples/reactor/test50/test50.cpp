#include "myself/communicate/serial/SerialClient.h"
#include "myself/reactor/EventLoopThread.h"
#include "myself/reactor/EventLoop.h"

#include <iostream>
#include <stdlib.h>

using namespace std;
using namespace myself;

int main()
{
    reactor::EventLoopThread loopThread;
    string deviceAddress("/dev/ttyUSB0");

    auto loopptr = loopThread.startLoop();

    mutex mutex_;
    serial::ConnectionPtr connection_;

    {
        serial::SerialClient client(loopptr, deviceAddress, "Test50");
        client.setConnectionCallback([&](const serial::ConnectionPtr& conn){

            printf("connection: %s -> %s is %s\n",
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

        client.setMessageCallback([&](const serial::ConnectionPtr& conn, Buffer* buffer){
            std::string data(buffer->peek(), buffer->peek() + buffer->readableBytes());
            buffer->retrieveAll();
            printf("message: %s\n", data.c_str());
            // conn->send("hello " + data + " world");
        });
        client.enableRetry();
        client.connect();

        auto timerId = loopptr->runEvery(1, [&]{
            if(connection_)
            {
                connection_->send("hello world");
            }
        });

        // 进入业务线程范围
        std::string line;
        while (std::getline(std::cin, line))
        {
            lock_guard<mutex> lock(mutex_);
            if(connection_)
            {
                // connection_->send(line);
                loopptr->cancel(timerId);
            }
        }
        client.disconnect();
    }
}
