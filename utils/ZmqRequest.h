#pragma once
#include "zmq.hpp"
#include <string>

namespace ems
{
using namespace std;

class ZmqRequest
{
public:
    ZmqRequest(const string& _addr);
    bool send(const vector<byte>& data);
    std::optional<vector<byte>> recv();
private:
    void init();
    const string addr_;
    zmq::context_t context_;
    zmq::socket_t socket_;
};

}//namespace ems
