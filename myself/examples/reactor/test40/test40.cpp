#include "myself/communicate/domain/DomainClient.h"
#include "myself/communicate/domain/DomainServer.h"
#include "myself/reactor/EventLoopThread.h"
#include "myself/reactor/EventLoop.h"

#include <iostream>
#include <stdlib.h>

using namespace std;
using namespace myself;

int main()
{
    reactor::EventLoopThread loopThread;
    string listenAddr("/var/tmp/domain_test_001");

    auto loopptr = loopThread.startLoop();
    domain::DomainServer server(loopptr, listenAddr);

    {
        server.setConnectionCallback([](const domain::ConnectionPtr& conn){
            printf("---connection: %s -> %s is %s\n",
                conn->localAddress().c_str(),
                conn->peerAddress().c_str(),
                conn->connected() ? "UP" : "DOWN");
        });
        server.setMessageCallback([](const domain::ConnectionPtr& conn, Buffer* buffer){
            std::string data(buffer->peek(), buffer->peek() + buffer->readableBytes());
            buffer->retrieveAll();
            printf("---message: %s\n", data.c_str());
            conn->send("---hello " + data + "world");
        });
        server.start();
    }

    {

        mutex mutex_;
        domain::ConnectionPtr connection_;

        domain::DomainClient client(loopptr, listenAddr, "Test001");
        client.setConnectionCallback([&](const domain::ConnectionPtr& conn){

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

        client.setMessageCallback([&](const domain::ConnectionPtr& conn, Buffer* buffer){
            std::string data(buffer->peek(), buffer->peek() + buffer->readableBytes());
            buffer->retrieveAll();
            printf("message: %s\n", data.c_str());
            // conn->send("hello " + data + " world");
        });
        client.enableRetry();
        client.connect();

        // 进入业务线程范围
        std::string line;
        while (std::getline(std::cin, line))
        {
            lock_guard<mutex> lock(mutex_);
            if(connection_)
            {
                connection_->send(line);
            }
        }
        client.disconnect();
    }
}
