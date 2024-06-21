#pragma once
#include "zmq.hpp"
#include <string>

namespace ems
{
using namespace std;

class ZmqPublish
{
public:
    ZmqPublish(const string& _addr);
    bool send(const void*data, size_t len);
private:
    void init();
    const string addr_;
    zmq::context_t context_;
    zmq::socket_t socket_;
};

}//namespace ems