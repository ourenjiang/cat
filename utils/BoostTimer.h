#pragma once
#include "boost/asio.hpp"
#include "boost/assert.hpp"

namespace ems
{
using namespace std;
using namespace boost::asio;

using TimeoutCallback = function<void()>;

class BoostTimer
{
public:
    BoostTimer(const int64_t timeoutMs);
    BoostTimer(const TimeoutCallback& callback, const int64_t timeoutMs);
    ~BoostTimer();
    void setTimeoutCallback(const TimeoutCallback& callback){ timeoutCallback_ = callback; }
    void start();
private:
    void onTimeout(const boost::system::error_code &error);

    io_service ioService_;
    steady_timer timer_;
    TimeoutCallback timeoutCallback_;
    const int64_t timeoutMs_;
    std::thread ioThread_;
};

}//namespace ems
