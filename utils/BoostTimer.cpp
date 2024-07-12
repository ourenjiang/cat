#include "BoostTimer.h"

using namespace std;
using namespace ems;
using namespace boost;

BoostTimer::BoostTimer(const int64_t timeoutMs)
    : timeoutMs_(timeoutMs)
    , timer_(ioService_, asio::chrono::milliseconds(timeoutMs))
{

}

BoostTimer::BoostTimer(const TimeoutCallback& callback, const int64_t timeoutMs)
    : timeoutCallback_(callback)
    , timeoutMs_(timeoutMs)
    , timer_(ioService_, asio::chrono::milliseconds(timeoutMs))
{

}

BoostTimer::~BoostTimer()
{
    if(ioThread_.joinable())
        ioThread_.join();
}

void BoostTimer::start()
{
    BOOST_ASSERT(timeoutCallback_);
    ioThread_ = thread([&]{
        using namespace std::placeholders;
        timer_.async_wait(bind(&BoostTimer::onTimeout, this, _1));
        ioService_.run();// blocking
    });
}

void BoostTimer::onTimeout(const system::error_code &error)
{
    timeoutCallback_();
    // 重新注册定时器
    timer_.expires_from_now(asio::chrono::milliseconds(timeoutMs_));
    using namespace std::placeholders;
    timer_.async_wait(bind(&BoostTimer::onTimeout, this, _1));
}
