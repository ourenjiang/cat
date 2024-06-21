#pragma once
#include <thread>

namespace ems
{
using namespace std;

class HttpServer
{
public:
    ~HttpServer(){
        if(loopThread_.joinable()) loopThread_.join();
    }
    
    void start();
private:
    std::thread loopThread_;
};
}//namespace ems
