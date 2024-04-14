#pragma once
#include "myself/communicate/tcp/InetAddress.h"
#include "myself/communicate/tcp/Callbacks.h"
#include "myself/base/Buffer.h"
#include "myself/base/Timestamp.h"

#include <string>
#include <functional>
#include <memory>

namespace myself
{
namespace tcp
{

class Socket;

class TcpConnection
    : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, const std::string& name,
        int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() { return localAddr_; }
    const InetAddress& peerAddress() { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }

    void send(const void* message, size_t len);
    // void send(const StringPiece& message);
    void send(const std::string& message);
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }

    void connectEstablished();
    void connectDestroyed();

private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

    void setState(StateE s) { state_ = s; }
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    // void sendInLoop(const StringPiece& message);
    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop* loop_;
    const std::string name_;
    StateE state_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    InetAddress localAddr_;
    InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};

}//namespace tcp
}//namespace myself
