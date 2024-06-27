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

namespace ems
{
namespace miscellaneous
{
using namespace std;
using namespace std::chrono;

// 解析IP&Port字符串
bool parseIpPortString(const std::string& ipPort, std::string& ip, int& port);
bool parseIpPortString(const std::string& ipPort, std::string& ip, std::string& port);

//CRC16计算
uint16_t getCrc16Modbus(void *data, uint16_t number);
uint16_t crc16_ibm(const uint8_t *src, uint32_t size);
//大小端转换
void reverseEndian(void *data, int len);
// ReverseEndian-Plus(use stl)
void reverseByteArray(void *arr, const unsigned long len);
// 从CGI获取POST参数
int getCgiPostPara(char *data);

//获取“月份-天”日期字符串
void getMonthDayString(std::chrono::system_clock::time_point t, std::string &dateStr);

//解析BCD码（压缩BCD码）
int32_t parseBCDGroupWithSymbol(void *bcd_mem, uint16_t byte_num, bool is_signed = false);
// 按指定精度序列化浮点数
string formatedPrecision(const double rawData, int formatedPrecision);
// 获取年-月-日 时:分:秒时间戮
string getCurrentTimestamp();
// 获取年-月-日 时间戮
string getCurrentYearMonthDay();
// 获取星期
string getCurrentWeekDay();
// 创建ModbusRTU读寄存器请求帧
vector<uint8_t> createModbusRtuReadFrame(const uint8_t devAddress, const uint8_t funcCode, const uint16_t regAddress, const uint16_t regNum);
vector<uint8_t> createModbusRtuWriteFrame(const uint8_t devAddress, const uint8_t funcCode, const uint16_t regAddress, const uint16_t regData);

// 反序列化json字符串
Json::Value unserializedJson(const string& jsonstring);
// 构造定长字符串
string createFixedSizeString(const string& content, const size_t len = 64);
// 构造响应消息
vector<byte> createRespondMessage(const bool status, const vector<byte>& content);
// 序列化JSON对象为字节流
vector<byte> serializedJsonAsBytes(const Json::Value& data);
// 字符串类型转换为字节流类型
vector<byte> convertStringToBytes(const string& data);
zmq::socket_t createZmqSocket(zmq::socket_type type);

// 强制ROOT权限
class AssertUserPriority
{
public:
    AssertUserPriority();
};

}////namespace miscellaneous
}//namespace ems
