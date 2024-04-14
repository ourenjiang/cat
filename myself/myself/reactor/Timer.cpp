#include "myself/reactor/Timer.h"

using namespace myself::reactor;

std::atomic<int64_t> Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
  if (repeat_)
  {
    expiration_ = addTime(now, interval_);
  }
  else
  {
    expiration_ = Timestamp::invalid();
  }
}
