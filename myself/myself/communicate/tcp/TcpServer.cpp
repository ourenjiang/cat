#include "myself/communicate/tcp/TcpServer.h"
#include "myself/communicate/tcp/Acceptor.h"
#include "myself/reactor/EventLoop.h"
#include "myself/base/SocketsOps.h"

#include <assert.h>

using namespace std;
using namespace myself;
using namespace myself::reactor;
using namespace myself::tcp;

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr)
    : loop_(loop)
    , hostport_(listenAddr.toHostPort())
    , acceptor_(new Acceptor(loop, listenAddr))
    , started_(false)
    , nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(
        bind(&TcpServer::newConnection, this, placeholders::_1, placeholders::_2));
}

TcpServer::~TcpServer()
{
    assert(loop_->isInLoopThread());
    // loop_->assertInLoopThread();
//   LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

  for (ConnectionMap::iterator it(connections_.begin());
      it != connections_.end(); ++it)
  {
    ConnectionPtr conn = it->second;
    it->second.reset();
    conn->getLoop()->runInLoop(bind(&TcpConnection::connectDestroyed, conn));
    conn.reset();
  }
}

void TcpServer::start()
{
    if (!started_)
    {
        started_ = true;
    }

    if (!acceptor_->listenning())
    {
        loop_->runInLoop(bind(&Acceptor::listen, acceptor_.get()));
    }
}


void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    assert(loop_->isInLoopThread());

    char buf[32];
    snprintf(buf, sizeof(buf), "#%d", nextConnId_);
    ++nextConnId_;
    string connName = buf;

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    ConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(bind(&TcpServer::removeConnection, this, placeholders::_1));

    loop_->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const ConnectionPtr& conn)
{
  loop_->runInLoop(bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const ConnectionPtr& conn)
{
    assert(loop_->isInLoopThread());

    size_t n = connections_.erase(conn->name());
    (void)n;
    assert(n == 1);
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(bind(&TcpConnection::connectDestroyed, conn));
}
