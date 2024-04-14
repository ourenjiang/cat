#pragma once
#include "myself/communicate/domain/Callbacks.h"

#include <memory>
#include <functional>
#include <string>

namespace myself
{
namespace domain
{

class DomainConnector
    : public std::enable_shared_from_this<DomainConnector>
{
 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;

  DomainConnector(EventLoop* loop, const std::string& serverAddr);
  ~DomainConnector();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  void start();  // can be called in any thread
  void restart();  // must be called in loop thread
  void stop();  // can be called in any thread

  const std::string& serverAddress() const { return serverAddr_; }

 private:
  enum States { kDisconnected, kConnecting, kConnected };
  // static const int kMaxRetryDelayMs = 30*1000;
  static const int kMaxRetryDelayMs = 5*1000;
  static const int kInitRetryDelayMs = 500;

  void setState(States s) { state_ = s; }
  void startInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWrite();
  void handleError();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

  bool isListenFileExist();

  EventLoop* loop_;
  std::string serverAddr_;
  bool connect_; // atomic
  States state_;  // FIXME: use atomic variable
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newConnectionCallback_;
  int retryDelayMs_;
};

}//namespace tcp
}//namespace myself
