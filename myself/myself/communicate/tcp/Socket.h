#pragma once

namespace myself
{
namespace tcp
{

class InetAddress;

class Socket
{
 public:
  explicit Socket(int sockfd): sockfd_(sockfd)
  {
  }

  ~Socket();

  int fd() const { return sockfd_; }

  void bindAddress(const InetAddress& localaddr);
  void listen();

  int accept(InetAddress* peeraddr);
  void shutdownWrite();
  void setTcpNoDelay(bool on);
  void setReuseAddr(bool on);
  void setKeepAlive(bool on);

 private:
  const int sockfd_;
};

}//namespace tcp
}//namespace myself
