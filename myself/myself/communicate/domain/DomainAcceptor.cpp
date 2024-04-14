#include "myself/communicate/domain/DomainAcceptor.h"
#include "myself/communicate/tcp/InetAddress.h"
#include "myself/reactor/EventLoop.h"
#include "myself/base/SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

using namespace std;
using namespace myself::reactor;
using namespace myself::domain;

DomainAcceptor::DomainAcceptor(EventLoop* loop, const string& listenAddr)
  : loop_(loop)
  , acceptSocket_(sockets::createDomainSocket())
  , acceptChannel_(loop, acceptSocket_.fd())
  , listenning_(false)
{
  deleteOldFile(listenAddr);
  // acceptSocket_.setReuseAddr(true);
  acceptSocket_.bindAddress(listenAddr);
  acceptChannel_.setReadCallback(bind(&DomainAcceptor::handleRead, this));
}

DomainAcceptor::~DomainAcceptor()
{
  // 这里会违背one loop per thread原则
  // 但如果使用runInLoop接口的话，会导致宿主会先于函数对象释放的问题；
  acceptChannel_.disableAll();
  acceptChannel_.remove();
}

void DomainAcceptor::listen()
{
  assert(loop_->isInLoopThread());
  listenning_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableReading();
}

void DomainAcceptor::handleRead()
{
  assert(loop_->isInLoopThread());
  struct sockaddr_un peerAddr;
  memset(&peerAddr, 0, sizeof peerAddr);
  int connfd = acceptSocket_.accept(&peerAddr);
  if (connfd >= 0)
  {
    if (newConnectionCallback_)
    {
      newConnectionCallback_(connfd, peerAddr.sun_path);
    }
    else
    {
      sockets::close(connfd);
    }
  }
  else
  {
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of livev.
    if (errno == EMFILE)
    {
    }
  }
}

void DomainAcceptor::deleteOldFile(const std::string& filename)
{
    // 检查域套接字的文件是否存在（默认情况下，服务器是负责初始化连接，所以由它负责删除旧的映射文件）
    if(::access(filename.c_str(), F_OK) == 0)
    {
      if(unlink(filename.c_str()) < 0)
      {
        perror("unlink");
        return;
      }
    }
}
