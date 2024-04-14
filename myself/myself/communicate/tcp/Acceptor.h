#pragma once
#include "myself/reactor/Channel.h"
#include "myself/communicate/tcp/Callbacks.h"
#include "myself/communicate/tcp/Socket.h"

namespace myself
{
namespace tcp
{

class InetAddress;

class Acceptor
{
 public:
  typedef std::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

  Acceptor(EventLoop *loop, const InetAddress& listenAddr);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  bool listenning() const { return listenning_; }
  void listen();

 private:
  void handleRead();

  EventLoop* loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool listenning_;
  int idleFd_;
};

}//namespace tcp
}//namespace myself
