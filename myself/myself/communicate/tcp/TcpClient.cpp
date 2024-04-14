#include "myself/communicate/tcp/TcpClient.h"
#include "myself/communicate/tcp/Connector.h"
#include "myself/reactor/EventLoop.h"
#include "myself/base/SocketsOps.h"

#include <stdio.h>  // snprintf

namespace myself
{
namespace tcp
{
namespace detail
{

void removeConnection(EventLoop* loop, const ConnectionPtr& conn)
{
  loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector)
{
  //connector->
}

}//namespace detail
}//namespace tcp
}//namespace myself

using namespace std;
using namespace myself;
using namespace myself::tcp;

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr,
                     const string& name)
  : loop_(loop),
    connector_(new Connector(loop, serverAddr)),
    name_(name),
    retry_(false),
    connect_(true),
    nextConnId_(1)
{
  connector_->setNewConnectionCallback(bind(&TcpClient::newConnection, this, placeholders::_1));
}

TcpClient::~TcpClient()
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
    loop_->runInLoop(bind(&TcpConnection::setCloseCallback, conn, cb));
  }
  else
  {
    connector_->stop();
    // FIXME: HACK
    loop_->runAfter(1, bind(&detail::removeConnector, connector_));
  }
}

void TcpClient::connect()
{
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect()
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

void TcpClient::stop()
{
  connect_ = false;
  connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
  assert(loop_->isInLoopThread());

  InetAddress peerAddr(sockets::getPeerAddr(sockfd));
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
  ++nextConnId_;
  string connName = name_ + buf;

  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
    ConnectionPtr conn(new TcpConnection(loop_,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(bind(&TcpClient::removeConnection, this, placeholders::_1)); // FIXME: unsafe
    {
        lock_guard<mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const ConnectionPtr& conn)
{
  assert(loop_->isInLoopThread());
  assert(loop_ == conn->getLoop());

  {
    lock_guard<mutex> lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  loop_->queueInLoop(bind(&TcpConnection::connectDestroyed, conn));
  if (retry_ && connect_)
  {
    connector_->restart();
  }
}
