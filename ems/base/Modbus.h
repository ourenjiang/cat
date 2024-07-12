#pragma once
#include <string.h>
#include <vector>
#include "utils/endian.h"

namespace modbus
{
using namespace std;

uint32_t getU32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);
int32_t getS32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);
uint16_t getU16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);
int16_t getS16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);


}//namespace modbus