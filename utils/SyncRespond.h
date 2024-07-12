#pragma once
#include <iostream>
#include "boost/asio.hpp"
#include <thread>
#include "boost/bind/bind.hpp"
#include <functional>
#include <map>

namespace ems
{
using namespace std;
using namespace boost::asio;

class SyncRespond
{
public:
    SyncRespond(const string port, function< vector<uint8_t> (const vector<uint8_t>&)> matchCallback);
    ~SyncRespond();

    bool asyncWrite(const int index, const std::string& data);

    void asyncRead(const int index);
private:
    void asyncAccept();

    void handle_accept(const boost::system::error_code& ec, 
                    ip::tcp::acceptor& acceptor, 
                    const int index, io_context& io_context);
    
    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred, const int index);

    void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred, const int index);

    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::io_context::work work_;
    map<int, pair<ip::tcp::socket, string>> sockets_;
    function< vector<uint8_t> (const vector<uint8_t>&)> respondMatchCallback_;
    int connectionNum_;
    std::thread ioContextThread_;
};

}//namespace ems
