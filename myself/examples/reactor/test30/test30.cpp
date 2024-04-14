#include "myself/reactor/EventLoopThread.h"
#include "myself/communicate/tcp/TcpClient.h"

#include <iostream>
#include <stdlib.h>

using namespace std;
using namespace myself;

int main()
{
    reactor::EventLoopThread loopThread;
    tcp::InetAddress serverAddr("192.168.49.1", 8101);

    mutex mutex_;
    tcp::ConnectionPtr connection_;

    tcp::TcpClient client(loopThread.startLoop(), serverAddr, "ChatClient");
    client.setConnectionCallback([&](const tcp::ConnectionPtr& conn){

        printf("connection: %s -> %s is %s\n",
            conn->localAddress().toIpPort().c_str(),
            conn->peerAddress().toIpPort().c_str(),
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

    client.setMessageCallback([&](const tcp::ConnectionPtr& conn,
                 Buffer* buffer){
        std::string data(buffer->peek(), buffer->peek() + buffer->readableBytes());
        buffer->retrieveAll();
        printf("message: %s\n", data.c_str());
        conn->send("hello " + data + " world");
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
