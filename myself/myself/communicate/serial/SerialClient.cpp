#include "myself/communicate/serial/SerialClient.h"
#include "myself/communicate/serial/SerialConnector.h"
#include "myself/reactor/EventLoop.h"
#include "myself/base/SocketsOps.h"

#include <stdio.h>  // snprintf

namespace myself
{
namespace serial
{
namespace detail
{

void removeConnection(EventLoop* loop, const ConnectionPtr& conn)
{
  loop->queueInLoop(std::bind(&SerialConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector)
{
  //connector->
}

}//namespace detail
}//namespace serial
}//namespace myself

using namespace std;
using namespace myself;
using namespace myself::serial;

SerialClient::SerialClient(EventLoop* loop,
                     const string& deviceAddress,
                     const string& name)
  : loop_(loop),
    connector_(new SerialConnector(loop, deviceAddress)),
    name_(name),
    retry_(false),
    connect_(true),
    nextConnId_(1)
{
  connector_->setNewConnectionCallback(bind(&SerialClient::newConnection, this, placeholders::_1));
}

SerialClient::~SerialClient()
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
    loop_->runInLoop(bind(&SerialConnection::setCloseCallback, conn, cb));
  }
  else
  {
    connector_->stop();
    // FIXME: HACK
    loop_->runAfter(1, bind(&detail::removeConnector, connector_));//为什么bind提示参数类型不匹配？？？
  }
}

void SerialClient::connect()
{
  connect_ = true;
  connector_->start();
}

void SerialClient::disconnect()
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

void SerialClient::stop()
{
  connect_ = false;
  connector_->stop();
}

void SerialClient::newConnection(int sockfd)
{
    assert(loop_->isInLoopThread());

    string peerAddr;
    // char buf[32];
    // snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    // string connName = name_ + buf;
    string connName = name_;

    string localAddr;
    ConnectionPtr conn(new SerialConnection(loop_,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setCloseCallback(bind(&SerialClient::removeConnection, this, placeholders::_1)); // FIXME: unsafe
    {
        lock_guard<mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void SerialClient::removeConnection(const ConnectionPtr& conn)
{
  assert(loop_->isInLoopThread());
  assert(loop_ == conn->getLoop());

  {
    lock_guard<mutex> lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  loop_->queueInLoop(bind(&SerialConnection::connectDestroyed, conn));
  if (retry_ && connect_)
  {
    connector_->restart();
  }
}
