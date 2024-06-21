#include "ZmqSubscribe.h"
#include <iostream>

using namespace ems;

ZmqSubscribe::ZmqSubscribe(const string& _addr)
    : addr_(_addr)
    , context_(1)
{
    init();
}

void ZmqSubscribe::subscribe(const string& topic)
{
    socket_.set(zmq::sockopt::subscribe, topic);
}

void ZmqSubscribe::unsubscribe(const string& topic)
{
    socket_.set(zmq::sockopt::unsubscribe, topic);
}

bool ZmqSubscribe::recv(string &msg)
{
    zmq::message_t message;
    zmq::recv_result_t result = socket_.recv(message, zmq::recv_flags::none);
    if(result.has_value()){
        // char* dataptr = static_cast<char*>(message.data());
        // msg = std::string{ dataptr, dataptr + message.size() };
        std::string(static_cast<char*>(message.data()), message.size()).swap(msg);
        return true;
    }
    else{// timeout
        /**
         * 在响应端跟踪接收超时状态，主要是为了方便调试，
         * 对于来自外部的定时请求，如果出现请求端的接收异常，可以配置此处的接收超时日志进行调试。
        */
        cout << "recv timeout" << endl;
    }
    return false;
}

void ZmqSubscribe::init()
{
    socket_ = zmq::socket_t(context_, zmq::socket_type::sub);
    // socket_.set(zmq::sockopt::linger, 0);
    // socket_.set(zmq::sockopt::rcvtimeo, 3000);
    socket_.connect(addr_);
}
