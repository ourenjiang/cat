#pragma once
#include <sstream>
#include <vector>
#include <stdint.h>

namespace modbus
{
using namespace std;

// 创建ModbusRTU读寄存器请求帧
vector<uint8_t> modbusRtuReadFrame(const uint8_t devAddress,
    const uint8_t funcCode, const uint16_t regAddress, const uint16_t regNum);
vector<uint8_t> modbusRtuWriteFrame(const uint8_t devAddress,
    const uint8_t funcCode, const uint16_t regAddress, const uint16_t regData);

}// namespace modbus
