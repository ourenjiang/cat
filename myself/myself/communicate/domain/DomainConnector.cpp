#include "myself/communicate/domain/DomainConnector.h"
#include "myself/reactor/Channel.h"
#include "myself/reactor/EventLoop.h"
#include "myself/base/SocketsOps.h"

#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>

using namespace std;
using namespace myself;
using namespace myself::reactor;
using namespace myself::domain;

const int DomainConnector::kMaxRetryDelayMs;

DomainConnector::DomainConnector(EventLoop* loop, const string& serverAddr)
  : loop_(loop),
    serverAddr_(serverAddr),
    connect_(false),
    state_(kDisconnected),
    retryDelayMs_(kInitRetryDelayMs)
{
}

DomainConnector::~DomainConnector()
{
}

void DomainConnector::start()
{
  connect_ = true;
  loop_->runInLoop(bind(&DomainConnector::startInLoop, this)); // FIXME: unsafe
}

void DomainConnector::startInLoop()
{
  assert(loop_->isInLoopThread());
  assert(state_ == kDisconnected);
  if (connect_)
  {
    connect();
  }
  else
  {
  }
}

void DomainConnector::connect()
{
  int sockfd = sockets::createDomainSocket();
  if(!isListenFileExist())
  {
    retry(sockfd);
  }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, serverAddr_.c_str(), serverAddr_.size());
    socklen_t addrlen = static_cast<socklen_t>(sizeof addr);

  int ret = ::connect(sockfd, (struct sockaddr*)&addr, addrlen);
  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno)
  {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      connecting(sockfd);
      break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      retry(sockfd);
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      sockets::close(sockfd);
      break;
    default:
      sockets::close(sockfd);
      break;
  }
}

void DomainConnector::restart()
{
  assert(loop_->isInLoopThread());

  setState(kDisconnected);
  retryDelayMs_ = kInitRetryDelayMs;
  connect_ = true;
  startInLoop();
}

void DomainConnector::stop()
{
  connect_ = false;
  // FIXME: cancel timer
}

void DomainConnector::connecting(int sockfd)
{
  setState(kConnecting);
  assert(!channel_);
  channel_.reset(new Channel(loop_, sockfd));
  channel_->setWriteCallback(std::bind(&DomainConnector::handleWrite, this)); // FIXME: unsafe
  channel_->setErrorCallback(std::bind(&DomainConnector::handleError, this)); // FIXME: unsafe

  // channel_->tie(shared_from_this()); is not working,
  // as channel_ is not managed by shared_ptr
  channel_->enableWriting();
}

int DomainConnector::removeAndResetChannel()
{
  channel_->disableAll();
  channel_->remove();
  int sockfd = channel_->fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  loop_->queueInLoop(std::bind(&DomainConnector::resetChannel, this)); // FIXME: unsafe
  return sockfd;
}

void DomainConnector::resetChannel()
{
  channel_.reset();
}

void DomainConnector::handleWrite()
{
  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    if (err)
    {
      retry(sockfd);
    }
    else
    {
      setState(kConnected);
      if (connect_)
      {
        newConnectionCallback_(sockfd);
      }
      else
      {
        sockets::close(sockfd);
      }
    }
  }
  else
  {
    // what happened?
    assert(state_ == kDisconnected);
  }
}

void DomainConnector::handleError()
{
  assert(state_ == kConnecting);

  int sockfd = removeAndResetChannel();
  int err = sockets::getSocketError(sockfd);
  retry(sockfd);
}

void DomainConnector::retry(int sockfd)
{
  sockets::close(sockfd);
  setState(kDisconnected);
  if (connect_)
  {
    loop_->runAfter(retryDelayMs_/1000.0, std::bind(&DomainConnector::startInLoop, shared_from_this()));
    retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
  }
  else
  {
  }
}

bool DomainConnector::isListenFileExist()
{
    // 检查域套接字的文件是否存在（默认情况下，服务器是负责初始化连接，所以由它负责删除旧的映射文件）
    int ret = ::access(serverAddr_.c_str(), F_OK);
    return ret == 0;
}
