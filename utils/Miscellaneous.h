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

namespace miscellaneous
{
using namespace std;
using namespace std::chrono;

/**
 * @brief 
 * @param address IPV4网络地址字符串，格式：IP地址:端口号，比如 127.0.0.1:8000
 * @return 可选类型
 * --{ip, port}
 * @exception NoThrow
*/
optional<pair<string, string>> parseTcpAddress(const string& address);

//解析BCD码（压缩BCD码）
int32_t parseBCDGroupWithSymbol(void *bcd_mem, uint16_t byte_num, bool is_signed = false);

}////namespace miscellaneous
