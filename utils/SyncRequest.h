#pragma once
#include <iostream>
#include "boost/asio.hpp"
#include <thread>
#include "boost/bind/bind.hpp"
#include <condition_variable>
#include <optional>

namespace ems
{
using namespace std;
using namespace boost::asio;

/*
再添加一个连接状态检查并自动重连的内部线程.
*/

class SyncRequest
{
public:
    SyncRequest();
    SyncRequest(const SyncRequest& other){}
    SyncRequest(const string& address);
    ~SyncRequest();

    void setAddress(const string& address){ address_ = address; }
    void start();
    void syncWrite(const vector<byte>& data, int64_t timeout);
    optional<vector<byte>> syncRead(const int64_t timeout);
private:
    void tryAsyncConnect();
    
    shared_ptr<io_context> ioContext_;
    shared_ptr<io_context::work> ioWork_;
    shared_ptr<ip::tcp::socket> ioSocket_;

    mutex mtx_;// 只是同步工具，并不属于资源，所以不需要拷贝或移动;
    condition_variable cv_;// 只是同步工具，并不属于资源，所以不需要拷贝或移动;
    bool dataReady_;

    string address_;
    bool ioStarted_;
    bool connected_;
    std::string recvBuffer_;
    size_t recvBytes_;

    std::shared_ptr<std::thread> ioContextThread_;
};

}//namespace ems
