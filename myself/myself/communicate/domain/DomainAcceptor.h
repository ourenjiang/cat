#pragma once
#include "myself/communicate/domain/DomainSocket.h"
#include "myself/communicate/domain/Callbacks.h"
#include "myself/reactor/Channel.h"

namespace myself
{
namespace domain
{

class DomainAcceptor
{
 public:
  typedef std::function<void (int sockfd, const std::string&)> NewConnectionCallback;

  DomainAcceptor(EventLoop *loop, const std::string& listenAddr);
  ~DomainAcceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  bool listenning() const { return listenning_; }
  void listen();

 private:
  void handleRead();
  void deleteOldFile(const std::string& filename);

  EventLoop* loop_;
  DomainSocket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool listenning_;
};

}//namespace domain
}//namespace myself
