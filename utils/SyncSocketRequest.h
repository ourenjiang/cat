#pragma once
#include <iostream>
#include "boost/asio.hpp"
#include <thread>
#include "boost/bind/bind.hpp"
#include <condition_variable>

namespace ems
{
using namespace std;
using boost::asio::ip::tcp;

class SyncSocketRequest
{
public:
    SyncSocketRequest(const string ip, const string port);
    ~SyncSocketRequest();

    void asyncConnect();
    bool asyncWrite(const std::string& data);
    bool syncReadConditionVariable();
    std::string gerRecvBuffer(){ return recvBuffer_; }
private:
    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
    void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);

    boost::asio::io_context io_context_;
    boost::asio::io_context::work work_;
    tcp::socket socket_;

    bool connected_;
    mutex mtx_;
    condition_variable cv_;
    bool dataReady_;
    std::string recvBuffer_;
    std::string ip_;
    string port_;
    std::thread ioContextThread_;
};

}//namespace ems
