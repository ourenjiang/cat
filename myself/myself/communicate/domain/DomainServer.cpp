#include "myself/communicate/domain/DomainServer.h"
#include "myself/communicate/domain/DomainAcceptor.h"
#include "myself/reactor/EventLoop.h"
#include "myself/base/SocketsOps.h"

#include <assert.h>

using namespace std;
using namespace myself;
using namespace myself::reactor;
using namespace myself::domain;

DomainServer::DomainServer(EventLoop* loop, const string& listenAddr)
    : loop_(loop)
    , listenAddr_(listenAddr)
    , acceptor_(new DomainAcceptor(loop, listenAddr))
    , started_(false)
    , nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(
        bind(&DomainServer::newConnection, this, placeholders::_1, placeholders::_2));
}

DomainServer::~DomainServer()
{
    assert(loop_->isInLoopThread());
    // loop_->assertInLoopThread();
//   LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for (ConnectionMap::iterator it(connections_.begin());
        it != connections_.end(); ++it)
    {
        ConnectionPtr conn = it->second;
        it->second.reset();
        conn->getLoop()->runInLoop(bind(&DomainConnection::connectDestroyed, conn));
        conn.reset();
    }
}

void DomainServer::start()
{
    if (!started_)
    {
        started_ = true;
    }

    if (!acceptor_->listenning())
    {
        loop_->runInLoop(bind(&DomainAcceptor::listen, acceptor_.get()));
    }
}


void DomainServer::newConnection(int sockfd, const string& peerAddr)
{
    assert(loop_->isInLoopThread());

    char buf[32];
    snprintf(buf, sizeof(buf), "#%d", nextConnId_);
    ++nextConnId_;
    string connName = buf;

    string localAddr(listenAddr_);
    ConnectionPtr conn(new DomainConnection(loop_, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(bind(&DomainServer::removeConnection, this, placeholders::_1));

    loop_->runInLoop(std::bind(&DomainConnection::connectEstablished, conn));
}

void DomainServer::removeConnection(const ConnectionPtr& conn)
{
  loop_->runInLoop(bind(&DomainServer::removeConnectionInLoop, this, conn));
}

void DomainServer::removeConnectionInLoop(const ConnectionPtr& conn)
{
    assert(loop_->isInLoopThread());

    size_t n = connections_.erase(conn->name());
    (void)n;
    assert(n == 1);
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(bind(&DomainConnection::connectDestroyed, conn));
}
