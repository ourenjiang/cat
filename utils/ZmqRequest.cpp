#include "ZmqRequest.h"

using namespace ems;

ZmqRequest::ZmqRequest(const string& _addr)
    : addr_(_addr)
    , context_(1)
{
    init();
}

bool ZmqRequest::send(const vector<byte>& data)
{
    zmq::message_t message(data.data(), data.size());
    const auto result = socket_.send(message, zmq::send_flags::none);
    return result.has_value();
}

std::optional<vector<byte>> ZmqRequest::recv()
{
    zmq::pollitem_t item{ socket_, 0, ZMQ_POLLIN, 0 };
    zmq::poll(&item, 1, chrono::milliseconds(3000));// 接收超时3秒
    if(item.revents & ZMQ_POLLIN){
        zmq::message_t message;
        const auto result = socket_.recv(message, zmq::recv_flags::none);
        if(result.has_value()){
            return vector<byte>(reinterpret_cast<byte*>(message.data()),
                                reinterpret_cast<byte*>(message.data()) + message.size());
        }
    }
    // printf("重置套接字\n");
    init();// 重置套接字
    return {};
}

void ZmqRequest::init()
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
