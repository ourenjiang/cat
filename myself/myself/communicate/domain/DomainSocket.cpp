#include "myself/communicate/domain/DomainSocket.h"
#include "myself/base/SocketsOps.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>  // bzero
#include <unistd.h>

using namespace std;
using namespace myself;
using namespace myself::domain;

DomainSocket::DomainSocket(int sockfd)
  : sockfd_(sockfd)
{
}

DomainSocket::~DomainSocket()
{
  sockets::close(sockfd_);
}

void DomainSocket::bindAddress(const string& addr)
{
    listenAddr_ = addr;

    struct sockaddr_un unaddr;
    memset(&unaddr, 0, sizeof unaddr);
    unaddr.sun_family = AF_UNIX;
    strncpy(unaddr.sun_path, addr.c_str(), addr.size());
    socklen_t unaddrlen = static_cast<socklen_t>(sizeof unaddr);
    ::bind(sockfd_, (struct sockaddr*)&unaddr, unaddrlen);
}

void DomainSocket::listen()
{
  sockets::listenOrDie(sockfd_);
}

int DomainSocket::accept(struct sockaddr_un* peeraddr)
{
  socklen_t addrlen = sizeof *peeraddr;
  int connfd = ::accept4(sockfd_, (struct sockaddr*)peeraddr,
                          &addrlen, SOCK_NONBLOCK|SOCK_CLOEXEC);
  return connfd;
}

void DomainSocket::shutdownWrite()
{
  sockets::shutdownWrite(sockfd_);
}

void DomainSocket::setTcpNoDelay(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
               &optval, sizeof optval);
  // FIXME CHECK
}

void DomainSocket::setReuseAddr(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof optval);
  // FIXME CHECK
}

void DomainSocket::setKeepAlive(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
               &optval, sizeof optval);
  // FIXME CHECK
}
