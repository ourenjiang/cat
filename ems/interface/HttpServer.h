#pragma once
#include <thread>
#include "utils/HttpWrapper.h"

namespace ems
{
using namespace std;
using namespace httplib;

class HttpServer
{
public:
    HttpServer(const int port);
    ~HttpServer(){
        if(loopThread_.joinable()) loopThread_.join();
    }
    void setGetCallback(const string& url, const Server::Handler& callback);
    void setPostCallback(const string& url, const Server::Handler& callback);
    void setPutCallback(const string& url, const Server::Handler& callback);
    void setDeleteCallback(const string& url, const Server::Handler& callback);
    void start();
private:
    int port_;
    httplib::Server& server_;
    std::thread loopThread_;
};
}//namespace ems
