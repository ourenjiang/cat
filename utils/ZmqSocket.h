#pragma once
#include "zmq.hpp"
#include <string>

namespace ems
{
using namespace std;

class ZmqSocket
{
public:
    ZmqSocket(const string& _addr);
    bool recv(string& ostring);
    zmq::socket_t& socket(){ return socket_; }
private:
    virtual void init() = 0;
    const string addr_;
    zmq::context_t context_;
    zmq::socket_t socket_;
};

}//namespace ems
