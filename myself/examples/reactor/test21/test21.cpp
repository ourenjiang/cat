#include "myself/reactor/EventLoop.h"
#include "myself/communicate/tcp/TcpServer.h"

#include <iostream>
#include <unistd.h>

using namespace std;
using namespace myself;

int main()
{
    tcp::InetAddress listenAddr(8100);
    reactor::EventLoop loop;

    tcp::TcpServer server(&loop, listenAddr);
    server.setConnectionCallback([](const tcp::ConnectionPtr& conn){
        printf("connection: %s -> %s is %s\n",
            conn->localAddress().toIpPort().c_str(),
            conn->peerAddress().toIpPort().c_str(),
            conn->connected() ? "UP" : "DOWN");
    });
    server.setMessageCallback([](const tcp::ConnectionPtr& conn, Buffer* buffer){
        std::string data(buffer->peek(), buffer->peek() + buffer->readableBytes());
        buffer->retrieveAll();
        printf("message: %s\n", data.c_str());
        conn->send("hello " + data + "world");
    });
    server.start();
    loop.loop();
}
