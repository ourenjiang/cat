#include "myself/communicate/tcp/Acceptor.h"
#include "myself/communicate/tcp/InetAddress.h"
#include "myself/reactor/EventLoop.h"

#include <iostream>
#include <unistd.h>

using namespace std;
using namespace myself;

void newConnection(int sockfd, const tcp::InetAddress& peerAddr)
{
    printf("newConnection from: %s\n", peerAddr.toHostPort().c_str());
    ::write(sockfd, "hello world", 11);
    ::close(sockfd);
}

int main()
{
    tcp::InetAddress listenAddr(8100);
    reactor::EventLoop loop;

    tcp::Acceptor acceptor(&loop, listenAddr);
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();

    loop.loop();
}
