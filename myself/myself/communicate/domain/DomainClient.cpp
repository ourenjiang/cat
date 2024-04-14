#include "myself/communicate/domain/DomainClient.h"
#include "myself/communicate/domain/DomainConnector.h"
#include "myself/reactor/EventLoop.h"
#include "myself/base/SocketsOps.h"

#include <stdio.h>  // snprintf

namespace myself
{
namespace domain
{
namespace detail
{

void removeConnection(EventLoop* loop, const ConnectionPtr& conn)
{
  loop->queueInLoop(std::bind(&DomainConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector)
{
  //connector->
}

}
}
}

using namespace std;
using namespace myself;
using namespace myself::domain;

DomainClient::DomainClient(EventLoop* loop,
                     const string& serverAddr,
                     const string& name)
  : loop_(loop),
    connector_(new DomainConnector(loop, serverAddr)),
    name_(name),
    retry_(false),
    connect_(true),
    nextConnId_(1)
{
  connector_->setNewConnectionCallback(bind(&DomainClient::newConnection, this, placeholders::_1));
}

DomainClient::~DomainClient()
{
  ConnectionPtr conn;
  {
    lock_guard<mutex> lock(mutex_);
    conn = connection_;
  }
  if (conn)
  {
    // FIXME: not 100% safe, if we are in different thread
    CloseCallback cb = bind(&detail::removeConnection, loop_, placeholders::_1);
    loop_->runInLoop(bind(&DomainConnection::setCloseCallback, conn, cb));
  }
  else
  {
    connector_->stop();
    // FIXME: HACK
    loop_->runAfter(1, bind(&detail::removeConnector, connector_));//为什么bind提示参数类型不匹配？？？
  }
}

void DomainClient::connect()
{
  connect_ = true;
  connector_->start();
}

void DomainClient::disconnect()
{
  connect_ = false;

  {
    lock_guard<mutex> lock(mutex_);
    if (connection_)
    {
      connection_->shutdown();
    }
  }
}

void DomainClient::stop()
{
  connect_ = false;
  connector_->stop();
}

void DomainClient::newConnection(int sockfd)
{
    assert(loop_->isInLoopThread());

    string peerAddr;
    // char buf[32];
    // snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    // string connName = name_ + buf;
    string connName = name_;

    string localAddr;
    ConnectionPtr conn(new DomainConnection(loop_,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(bind(&DomainClient::removeConnection, this, placeholders::_1)); // FIXME: unsafe
    {
        lock_guard<mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void DomainClient::removeConnection(const ConnectionPtr& conn)
{
  assert(loop_->isInLoopThread());
  assert(loop_ == conn->getLoop());

  {
    lock_guard<mutex> lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  loop_->queueInLoop(bind(&DomainConnection::connectDestroyed, conn));
  if (retry_ && connect_)
  {
    connector_->restart();
  }
}
