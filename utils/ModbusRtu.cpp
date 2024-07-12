#include "ModbusRtu.h"
#include <cstring>
#include "crc16.h"

using namespace std;

vector<uint8_t> modbus::modbusRtuReadFrame(const uint8_t devAddress, const uint8_t funcCode, const uint16_t regAddress, const uint16_t regNum)
{
    vector<uint8_t> frame(8);
    frame[0] = devAddress;
    frame[1] = funcCode;

    const uint16_t regAddressBigEndian = htobe16(regAddress);
    ::memcpy(frame.data() + 2, &regAddressBigEndian, sizeof(uint16_t));
    const uint16_t regNumBigEndian = htobe16(regNum);
    ::memcpy(frame.data() + 4, &regNumBigEndian, sizeof(uint16_t));
    const uint16_t crc16Modbus = modbus::crc16_manual(frame.data(), 6);
    ::memcpy(frame.data() + 6, &crc16Modbus, sizeof(uint16_t));
    return frame;
}

vector<uint8_t> modbus::modbusRtuWriteFrame(const uint8_t devAddress, const uint8_t funcCode, const uint16_t regAddress, const uint16_t regData)
{
    vector<uint8_t> frame(8);
    frame[0] = devAddress;
    frame[1] = funcCode;

    const uint16_t regAddressBigEndian = htobe16(regAddress);
    ::memcpy(frame.data() + 2, &regAddressBigEndian, sizeof(uint16_t));
    const uint16_t regDataBigEndian = htobe16(regData);
    ::memcpy(frame.data() + 4, &regDataBigEndian, sizeof(uint16_t));
    const uint16_t crc16Modbus = modbus::crc16_manual(frame.data(), 6);
    ::memcpy(frame.data() + 6, &crc16Modbus, sizeof(uint16_t));
    return frame;
}
