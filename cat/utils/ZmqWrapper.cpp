#include "ZmqWrapper.h"
#include <iostream>
using namespace zmq_wrapper;

SocketRequest::SocketRequest(const string& _addr)
    : addr_(_addr)
    , context_(1)
{
    init();
}

bool SocketRequest::send(const void *data, size_t len)
{
    zmq::message_t message(data, len);
    zmq::send_result_t result = socket_.send(message, zmq::send_flags::none);
    return result.has_value();
}

bool SocketRequest::recv(string &ostring)
{
    zmq::pollitem_t item{ socket_, 0, ZMQ_POLLIN, 0 };
    zmq::poll(&item, 1, chrono::milliseconds(1000));// 接收超时1秒
    if(item.revents & ZMQ_POLLIN)
    {
        zmq::message_t message;
        zmq::recv_result_t result = socket_.recv(message, zmq::recv_flags::none);
        if(result.has_value())
        {
            ostring = std::string(static_cast<char*>(message.data()), message.size());
            return true;
        }
    }
    init();// 重置套接字
    return false;
}

void SocketRequest::init()
{
    if(socket_)
    {
        socket_.close();
        usleep(1000 * 50);// 等待后台资源释放
    }
    socket_ = zmq::socket_t(context_, zmq::socket_type::req);
    socket_.set(zmq::sockopt::linger, 0);
    socket_.connect(addr_);
}

SocketRespond::SocketRespond(const string _addr)
    : addr_(_addr)
    , context_(1)
{
    init();
}

bool SocketRespond::send(const void *data_, size_t size_)
{
    zmq::message_t msg(data_, size_);
    zmq::send_result_t result = socket_.send(msg, zmq::send_flags::none);
    return result.has_value();
}

bool SocketRespond::recv(string& msg)
{
    zmq::pollitem_t item{ socket_, 0, ZMQ_POLLIN, 0 };
    zmq::poll (&item, 1, std::chrono::milliseconds(1000));
    if(item.revents & ZMQ_POLLIN)
    {
        zmq::message_t message;
        zmq::recv_result_t result = socket_.recv(message, zmq::recv_flags::none);
        if(result.has_value())
        {
            char* dataptr = static_cast<char*>(message.data());
            msg = std::string{ dataptr, dataptr + message.size() };
            return true;
        }
    }
    init();// 接收失败, 重置套接字
    return false;
}

void SocketRespond::init()
{
    if(socket_)
    {
        socket_.close();
        usleep(1000 * 50);// 等待后台资源释放
    }
    socket_ = zmq::socket_t(context_, zmq::socket_type::rep);
    socket_.set(zmq::sockopt::linger, 0);// 关闭时不等待，直接退出
    socket_.bind(addr_);// 绑定地址
}
