#include "Modbus.h"

using namespace std;

uint32_t modbus::getU32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    uint32_t point{};
    ::memcpy(&point, frameRegisters.data() + (addr - beginAddr), sizeof(uint32_t));
    endian::reverse(&point, sizeof(uint32_t));
    return point;
}

int32_t modbus::getS32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    int32_t point{};
    ::memcpy(&point, frameRegisters.data() + (addr - beginAddr), sizeof(int32_t));
    endian::reverse(&point, sizeof(int32_t));
    return point;
}

uint16_t modbus::getU16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    uint16_t point{};
    ::memcpy(&point, frameRegisters.data() + (addr - beginAddr), sizeof(uint16_t));
    endian::reverse(&point, sizeof(uint16_t));
    return point;
}

int16_t modbus::getS16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    int16_t point{};
    ::memcpy(&point, frameRegisters.data() + (addr - beginAddr), sizeof(int16_t));
    endian::reverse(&point, sizeof(int16_t));
    return point;
}