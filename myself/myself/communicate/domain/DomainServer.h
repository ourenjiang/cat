#pragma once
#include "myself/communicate/domain/DomainConnection.h"
#include "myself/base/Timestamp.h"

#include <string>
#include <memory>
#include <functional>
#include <map>

namespace myself
{
namespace domain
{

class DomainAcceptor;

class DomainServer
{
public:
    DomainServer(EventLoop* loop, const std::string& listenAddr);
    ~DomainServer();
    void start();
    void setConnectionCallback(const ConnectionCallback& cb){ connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb){ messageCallback_ = cb; }
private:
    void newConnection(int sockfd, const std::string& peerAddr);
    void removeConnection(const ConnectionPtr& conn);
    void removeConnectionInLoop(const ConnectionPtr& conn);

    EventLoop* loop_;  // the acceptor loop
    const std::string listenAddr_;
    std::unique_ptr<DomainAcceptor> acceptor_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;

    bool started_;
    int nextConnId_;
    
    typedef std::map<std::string, ConnectionPtr> ConnectionMap;
    ConnectionMap connections_;
};

}//namespace tcp
}//namespace myself