#pragma once
#include "zmq.hpp"
#include <string>

namespace ems
{
using namespace std;

class ZmqRespond
{
public:
    using string = std::string;

    ZmqRespond(const string _addr);
    bool send(const vector<byte>& data);
    std::optional<vector<byte>> recv();
    zmq::socket_t& socket(){ return socket_; }
private:
    void init();
    const std::string addr_;
    zmq::context_t context_;
    zmq::socket_t socket_;
};

}//namespace ems
