#include "Simulator.h"
#include "utils/Miscellaneous.h"
#include <cstring>

using namespace ems::pcs;

Simulator::Simulator()
{
    create_0406_0460();
    create_0474_04D0();
}

void Simulator::create_0406_0460()
{
    // 创建请求帧
    vector<uint8_t> requestFrame(8);
    requestFrame[0] = 0x01;
    requestFrame[1] = 0x03;
    {
        uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
        *registerAddrPtr = 0x0406;
        miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    }
    {
        uint16_t* registerNumPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
        *registerNumPtr = 0x0460 - 0x0406 + 1;
        miscellaneous::reverseByteArray(registerNumPtr, sizeof(uint16_t));
    }
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestFrame.data(), 6);
        std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    }

    // 创建响应帧
    vector<uint8_t> respondFrame(5 + sizeof(uint16_t) * (0x0460 - 0x0406 + 1));
    respondFrame[0] = 0x01;
    respondFrame[1] = 0x03;
    respondFrame[2] = 2 + sizeof(uint16_t) * (0x0460 - 0x0406 + 1);
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(respondFrame.data(), 
                                                                respondFrame.size() - sizeof(uint16_t));
        uint8_t *crc16Ptr = respondFrame.data() + respondFrame.size() - sizeof(uint16_t);
        std::memcpy(crc16Ptr, &crc16Modbus, sizeof(uint16_t));
    }

    insertCommunicateInstance(requestFrame, respondFrame);

}

void Simulator::create_0474_04D0()
{
    // 创建请求帧
    vector<uint8_t> requestFrame(8);
    requestFrame[0] = 0x01;
    requestFrame[1] = 0x03;
    {
        uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
        *registerAddrPtr = 0x0474;
        miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    }
    {
        uint16_t* registerNumPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
        *registerNumPtr = 0x04D0 - 0x0474 + 1;
        miscellaneous::reverseByteArray(registerNumPtr, sizeof(uint16_t));
    }
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestFrame.data(), 6);
        std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    }

    // 创建响应帧
    vector<uint8_t> respondFrame(5 + sizeof(uint16_t) * (0x04D0 - 0x0474 + 1));
    respondFrame[0] = 0x01;
    respondFrame[1] = 0x03;
    respondFrame[2] = 2 + sizeof(uint16_t) * (0x04D0 - 0x0474 + 1);
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(respondFrame.data(), 
                                                                respondFrame.size() - sizeof(uint16_t));
        uint8_t *crc16Ptr = respondFrame.data() + respondFrame.size() - sizeof(uint16_t);
        std::memcpy(crc16Ptr, &crc16Modbus, sizeof(uint16_t));
    }

    insertCommunicateInstance(requestFrame, respondFrame);
}

void Simulator::insertCommunicateInstance(const vector<uint8_t>& request, const vector<uint8_t>& respond)
{
    requestRespondMap_.emplace(request, respond);
}

vector<uint8_t> Simulator::getRespondFrame(const vector<uint8_t>& request)
{
    auto itr = requestRespondMap_.find(request);
    if(itr != requestRespondMap_.end()){
        return itr->second;
    }

    // 对于未注册的请求，默认为都是0x06功能码的控制指定，原样返回。
    return request;
    // return {};
}
