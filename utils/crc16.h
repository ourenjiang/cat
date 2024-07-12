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

namespace modbus
{
using namespace std;

/**
 * @brief 纯手工计算CRC16
 * @param buffer 帧起始地址
 * @param buffer_length 帧长度
 * @return CRC16
 * @exception NoThrow
*/
uint16_t crc16_manual(const uint8_t *buffer, uint16_t buffer_length);

/**
 * @brief 半表查询CRC16
 * @param buffer 帧起始地址
 * @param buffer_length 帧长度
 * @return CRC16
 * @exception NoThrow
*/
uint16_t crc16_half(const uint8_t *buffer, uint16_t buffer_length);

/**
 * @brief 全表查询CRC16
 * @param buffer 帧起始地址
 * @param buffer_length 帧长度
 * @return CRC16
 * @exception NoThrow
*/
uint16_t crc16_full(const uint8_t *buffer, uint16_t buffer_length);

}////namespace modbus
