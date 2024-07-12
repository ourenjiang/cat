#pragma once
#include <sstream>
#include <vector>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <chrono>
#include "json/json.h"
#include <optional>
#include "zmq.hpp"
#include <regex>

namespace datetime
{
using namespace std;
using namespace std::chrono;

//获取“月份-天”日期字符串
void getMonthDayString(std::chrono::system_clock::time_point t, std::string &dateStr);
// 获取年-月-日 时:分:秒时间戮
string getCurrentTimestamp();
// 获取年-月-日 时间戮
string getCurrentYearMonthDay();
// 获取星期
string getCurrentWeekDay();

}////namespace datetime
