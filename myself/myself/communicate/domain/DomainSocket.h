#pragma once

#include <string>
#include <sys/un.h>

namespace myself
{
namespace domain
{

class DomainSocket
{
 public:
  explicit DomainSocket(int sockfd);
  ~DomainSocket();

  int fd() const { return sockfd_; }

  void bindAddress(const std::string& localaddr);
  void listen();

  int accept(struct sockaddr_un* peeraddr);
  void shutdownWrite();
  void setTcpNoDelay(bool on);
  void setReuseAddr(bool on);
  void setKeepAlive(bool on);

 private:
  const int sockfd_;
  std::string listenAddr_;
};

}//namespace domain
}//namespace myself
