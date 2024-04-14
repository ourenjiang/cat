#pragma once
#include "myself/communicate/serial/Callbacks.h"

#include <memory>
#include <functional>
#include <string>

namespace myself
{
namespace serial
{

class SerialConnector
    : public std::enable_shared_from_this<SerialConnector>
{
 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;

  SerialConnector(EventLoop* loop, const std::string& serialDevice);
  ~SerialConnector();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  void start();  // can be called in any thread
  void restart();  // must be called in loop thread
  void stop();  // can be called in any thread

  const std::string& deviceAddress() const { return deviceAddress_; }

 private:
  enum States { kDisconnected, kConnecting, kConnected };
  // static const int kMaxRetryDelayMs = 30*1000;
  static const int kMaxRetryDelayMs = 5*1000;
  static const int kInitRetryDelayMs = 500;

  void setState(States s) { state_ = s; }
  void startInLoop();
  void connect();
  void connecting(int serialfd);
  void handleWrite();
  void handleError();
  void retry(int serialfd);
  int removeAndResetChannel();
  void resetChannel();

  int openDeviceAddress();
  void setSerialOptions(int fd);

  EventLoop* loop_;
  std::string deviceAddress_;
  bool connect_; // atomic
  States state_;  // FIXME: use atomic variable
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newConnectionCallback_;
  int retryDelayMs_;
};

}//namespace tcp
}//namespace myself
