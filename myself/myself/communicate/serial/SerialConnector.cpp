#include "myself/communicate/serial/SerialConnector.h"
#include "myself/reactor/Channel.h"
#include "myself/reactor/EventLoop.h"
#include "myself/base/SocketsOps.h"

#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>
#include <termio.h>
#include <fcntl.h>

using namespace std;
using namespace myself;
using namespace myself::reactor;
using namespace myself::serial;

const int SerialConnector::kMaxRetryDelayMs;

SerialConnector::SerialConnector(EventLoop* loop, const string& deviceAddress)
  : loop_(loop)
  , deviceAddress_(deviceAddress)
  , connect_(false)
  , state_(kDisconnected)
  , retryDelayMs_(kInitRetryDelayMs)
{
}

SerialConnector::~SerialConnector()
{
}

void SerialConnector::start()
{
  connect_ = true;
  loop_->runInLoop(bind(&SerialConnector::startInLoop, this)); // FIXME: unsafe
}

void SerialConnector::startInLoop()
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

void SerialConnector::connect()
{
    int serialfd = openDeviceAddress();
    if(serialfd > 0)
    {
      connecting(serialfd);
    }
    else
    {
      retry(serialfd);
    }
}

void SerialConnector::restart()
{
  assert(loop_->isInLoopThread());

  setState(kDisconnected);
  retryDelayMs_ = kInitRetryDelayMs;
  connect_ = true;
  startInLoop();
}

void SerialConnector::stop()
{
  connect_ = false;
  // FIXME: cancel timer
}

void SerialConnector::connecting(int serialfd)
{
  setState(kConnecting);
  assert(!channel_);
  channel_.reset(new Channel(loop_, serialfd));
  channel_->setWriteCallback(std::bind(&SerialConnector::handleWrite, this)); // FIXME: unsafe
  channel_->setErrorCallback(std::bind(&SerialConnector::handleError, this)); // FIXME: unsafe
  channel_->enableWriting();
}

int SerialConnector::removeAndResetChannel()
{
  channel_->disableAll();
  channel_->remove();
  int serialfd = channel_->fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  loop_->queueInLoop(std::bind(&SerialConnector::resetChannel, this)); // FIXME: unsafe
  return serialfd;
}

void SerialConnector::resetChannel()
{
  channel_.reset();
}

void SerialConnector::handleWrite()
{
  if (state_ == kConnecting)
  {
    int serialfd = removeAndResetChannel();
    int err = sockets::getSocketError(serialfd);
    if (err)
    {
      retry(serialfd);
    }
    else
    {
      setState(kConnected);
      if (connect_)
      {
        newConnectionCallback_(serialfd);
      }
      else
      {
        ::close(serialfd);
      }
    }
  }
  else
  {
    // what happened?
    assert(state_ == kDisconnected);
  }
}

void SerialConnector::handleError()
{
  assert(state_ == kConnecting);

  int serialfd = removeAndResetChannel();
  int err = sockets::getSocketError(serialfd);
  retry(serialfd);
}

void SerialConnector::retry(int serialfd)
{
  if(serialfd > 0)
  {
    ::close(serialfd);
  }

  setState(kDisconnected);
  if (connect_)
  {
    loop_->runAfter(retryDelayMs_/1000.0, std::bind(&SerialConnector::startInLoop, shared_from_this()));
    retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
  }
  else
  {
  }
}

int SerialConnector::openDeviceAddress()
{
    // 检查域套接字的文件是否存在（默认情况下，服务器是负责初始化连接，所以由它负责删除旧的映射文件）
    int fd = 0;
    if(::access(deviceAddress_.c_str(), F_OK) == 0)
    {
      fd = ::open(deviceAddress_.c_str(), O_RDWR|O_NOCTTY|O_NDELAY);
      setSerialOptions(fd);
    }
    return fd;
}

void SerialConnector::setSerialOptions(int fd)
{
    struct termios options;
    memset(&options, 0, sizeof(options));
    // tcgetattr(fd, &options);

    // 波特率（输入&输出）
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    options.c_cflag |= (CLOCAL|CREAD);//忽略调制解调器线路状态，允许接收数据
    options.c_cflag &= ~PARENB;// 奇偶校验：无
    options.c_cflag &= ~CSTOPB; // 停止位：1
    options.c_cflag &= ~CSIZE;// 清除数据位
    options.c_cflag |= CS8;//数据位：8
    tcsetattr(fd, TCSANOW, &options);
}
