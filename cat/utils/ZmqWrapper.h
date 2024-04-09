#pragma once
#include "zmq.hpp"
#include <string>

namespace zmq_wrapper
{
using namespace std;

class SocketRequest
{
public:
    SocketRequest(const string& _addr);
    bool send(const void*data, size_t len);
    bool recv(string& ostring);
private:
    void init();
    const string addr_;
    zmq::context_t context_;
    zmq::socket_t socket_;
};

class SocketRespond
{
public:
    using string = std::string;

    SocketRespond(const string _addr);
    bool send(const void *data_, size_t size_);
    bool recv(string& msg);
private:
    void init();
    const std::string addr_;
    zmq::context_t context_;
    zmq::socket_t socket_;
};

}//namespace zmq_wrapper
