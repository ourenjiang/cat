#include "myself/communicate/tcp/TcpConnection.h"
#include "myself/communicate/tcp/Socket.h"
#include "myself/reactor/Channel.h"
#include "myself/reactor/EventLoop.h"
#include "myself/base/SocketsOps.h"

#include <assert.h>

using namespace std;
using namespace myself;
using namespace myself::reactor;
using namespace myself::tcp;

TcpConnection::TcpConnection(EventLoop* loop,
        const std::string& name, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
    : loop_(loop)
    , name_(name)
    , state_(kConnecting)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
}

TcpConnection::~TcpConnection()
{
}

void TcpConnection::send(const void* data, size_t len)
{
    // send(StringPiece(static_cast<const char*>(data), len));
    sendInLoop(data, len);
}

// void TcpConnection::send(const StringPiece& message)
void TcpConnection::send(const std::string& message)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      // sendInLoop(message);
      sendInLoop(message.data(), message.size());
    }
    else
    {
        // 为什么boost::bind可以绑定重载函数，而发展到std::bind就不行了呢？  因为bind不允许绑定有重载函数名
        // 这里转换成函数指针
        // void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
        // loop_->runInLoop(bind(fp, this, message.as_string()));
        loop_->runInLoop(bind(&TcpConnection::sendInLoop, this, message.data(), message.size()));
    }
  }
}

// void TcpConnection::sendInLoop(const StringPiece& message)
// {
//   sendInLoop(message.data(), message.size());
// }

void TcpConnection::sendInLoop(const void* data, size_t len)
{
  assert(loop_->isInLoopThread());
  
  ssize_t nwrote = 0;
  size_t remaining = len;
  // if no thing in output queue, try writing directly
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
  {
    nwrote = sockets::write(channel_->fd(), data, len);
    if (nwrote >= 0)
    {
        remaining = len - nwrote;
        // 省略写完回调
    }
    else // nwrote < 0
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
      }
    }
  }

  assert(remaining <= len);
  if (remaining > 0)
  {
    // 此处省略高水位回调
    outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
    if (!channel_->isWriting())
    {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdown()
{
  // FIXME: use compare and swap
  if (state_ == kConnected)
  {
    setState(kDisconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::connectEstablished()
{
    assert(loop_->isInLoopThread());

    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    if(connectionCallback_)
    {
        connectionCallback_(shared_from_this());
    }
}

/**
 * 注意：connectDestroyed有两种执行方式，
 *    方式1：直接由用户基于TcpConnection对象调用，
 *    方式2：内部触发
 * 
*/
void TcpConnection::connectDestroyed()
{
  assert(loop_->isInLoopThread());

  if (state_ == kConnected)
  {
    setState(kDisconnected);
    channel_->disableAll();

    if(connectionCallback_)
    {
        connectionCallback_(shared_from_this());
    }
  }
  channel_->remove();
}

void TcpConnection::handleRead()
{
    assert(loop_->isInLoopThread());

    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        if(messageCallback_)
        {
            messageCallback_(shared_from_this(), &inputBuffer_);
        }
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    assert(loop_->isInLoopThread());

    if (channel_->isWriting())
    {
        ssize_t n = sockets::write(channel_->fd(),
                                outputBuffer_.peek(),
                                outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
            else
            {

            }
        }
        else
        {

        }
    }
    else
    {
    }
}

void TcpConnection::handleClose()
{
    assert(loop_->isInLoopThread());
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll();

    ConnectionPtr guardThis(shared_from_this());
    if(connectionCallback_)
    {
        connectionCallback_(guardThis);
    }

    // must be the last line
    closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
}

void TcpConnection::shutdownInLoop()
{
  assert(loop_->isInLoopThread());

  if (!channel_->isWriting())
  {
    // we are not writing
    socket_->shutdownWrite();
  }
}
