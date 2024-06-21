#include "ZmqPublish.h"
#include <iostream>

using namespace ems;

ZmqPublish::ZmqPublish(const string& _addr)
    : addr_(_addr)
    , context_(1)
{
    init();
}

bool ZmqPublish::send(const void *data, size_t len)
{
    zmq::message_t message(data, len);
    zmq::send_result_t result = socket_.send(message, zmq::send_flags::none);
    // cout << "send size: " << result.value() << endl;
    return result.has_value();
}

void ZmqPublish::init()
{
    socket_ = zmq::socket_t(context_, zmq::socket_type::pub);
    // socket_.set(zmq::sockopt::linger, 0);
    socket_.bind(addr_);// 绑定地址
}
