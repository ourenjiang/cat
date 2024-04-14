#pragma once
#include "myself/communicate/tcp/InetAddress.h"
#include "myself/communicate/tcp/TcpConnection.h"
#include "myself/base/Timestamp.h"

#include <string>
#include <memory>
#include <functional>
#include <map>

namespace myself
{
namespace tcp
{

class Acceptor;

class TcpServer
{
public:
    TcpServer(EventLoop* loop, const InetAddress& listenAddr);
    ~TcpServer();
    void start();
    void setConnectionCallback(const ConnectionCallback& cb){ connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb){ messageCallback_ = cb; }
private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const ConnectionPtr& conn);
    void removeConnectionInLoop(const ConnectionPtr& conn);

    EventLoop* loop_;  // the acceptor loop
    const std::string hostport_;
    std::unique_ptr<Acceptor> acceptor_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;

    bool started_;
    int nextConnId_;
    
    typedef std::map<std::string, ConnectionPtr> ConnectionMap;
    ConnectionMap connections_;
};

}//namespace tcp
}//namespace myself