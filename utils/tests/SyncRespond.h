#pragma once
#include <iostream>
#include "boost/asio.hpp"
#include <thread>
#include "boost/bind/bind.hpp"
#include <functional>

namespace ems
{
using namespace std;
using namespace boost::asio;

class SyncRespond
{
public:
    SyncRespond(const string port, function< vector<uint8_t> (const vector<uint8_t>&)> matchCallback);
    ~SyncRespond();

    bool asyncWrite(const std::string& data);
    void asyncRead();
private:
    void asyncAccept();
    void handle_accept(const boost::system::error_code& ec, 
                    ip::tcp::acceptor& acceptor, ip::tcp::socket& connected_socket, io_context& io_context);
    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
    void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);

    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::io_context::work work_;
    ip::tcp::socket socket_;
    string recvBuffer_;
    function< vector<uint8_t> (const vector<uint8_t>&)> respondMatchCallback_;
    std::thread ioContextThread_;
};

}//namespace ems
