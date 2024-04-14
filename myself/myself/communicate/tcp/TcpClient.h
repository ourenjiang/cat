#pragma once
#include "myself/communicate/tcp/TcpConnection.h"

#include <mutex>

namespace myself
{
namespace tcp
{

class TcpClient
{
 public:
  TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& name);
  ~TcpClient();

  void connect();
  void disconnect();
  void stop();

  ConnectionPtr connection() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return connection_;
  }

//   bool retry() const;
  void enableRetry() { retry_ = true; }

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd);
  /// Not thread safe, but in loop
  void removeConnection(const ConnectionPtr& conn);

  EventLoop* loop_;
  ConnectorPtr connector_; // avoid revealing Connector
  const std::string name_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  bool retry_;   // atmoic
  bool connect_; // atomic
  // always in loop thread
  int nextConnId_;
  mutable std::mutex mutex_;// mutable 兼容connection()接口的const属性
  ConnectionPtr connection_; // @BuardedBy mutex_
};

}//namespace tcp
}//namespace myself
