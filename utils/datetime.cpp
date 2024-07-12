#include "datetime.h"
#include <regex>
// #include <iomanip>
#include "boost/assert.hpp"

using namespace std;

void datetime::getMonthDayString(system_clock::time_point local_time, std::string &dateStr)
{
    auto local_time_t = system_clock::to_time_t(local_time);
    auto format_time = gmtime(&local_time_t);

    char buf[8] = {0};
    strftime(buf, sizeof(buf), "%m-%d", format_time);
    dateStr = std::move(buf);
}

string datetime::getCurrentTimestamp()
{
    using Clock = std::chrono::system_clock;
    auto now = Clock::now();
    auto time_t_now = Clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&time_t_now), "%F %T");
    return ss.str();
}

string datetime::getCurrentYearMonthDay()
{
    using Clock = std::chrono::system_clock;
    auto now = Clock::now();
    auto time_t_now = Clock::to_time_t(now);
    stringstream ss;
    ss << put_time(localtime(&time_t_now), "%F");
    return ss.str();
}

string datetime::getCurrentWeekDay()
{
    using Clock = std::chrono::system_clock;
    auto now = Clock::now();
    auto time_t_now = Clock::to_time_t(now);
    const tm* tmPtr = localtime(&time_t_now);
    return std::to_string(tmPtr->tm_wday);// [0, 6]
}
