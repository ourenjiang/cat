#include "Simulator.h"
#include "utils/Miscellaneous.h"
#include <cstring>
#include <iostream>

using namespace std;
using namespace ems::bau;

Simulator::Simulator()
{
    create_bingjiStatus();
    create_bauStatus();
    create_bauPowerOff();
    create_bauQuickStartup();
    for(int i = 0; i < 3; ++i){
        create_bcuStatus(i);
        create_bcuSetRelayGridOff(i);
        create_bcuSetRelayGridOn(i);

        for(int j = 0; j < 12; j++){
            create_bmuCellvolt(i, j);
            create_bmuCelltem(i, j);
        }
    }
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
    return {};
}

void Simulator::create_bingjiStatus()
{
    // 创建请求帧
    vector<uint8_t> requestFrame(8);
    requestFrame[0] = 0x01;
    requestFrame[1] = 0x03;
    {
        uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
        *registerAddrPtr = 0x0200;
        miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    }
    {
        uint16_t* registerNumPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
        *registerNumPtr = 0x0241 - 0x0200 + 1;
        miscellaneous::reverseByteArray(registerNumPtr, sizeof(uint16_t));
    }
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestFrame.data(), 6);
        std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    }

    // 创建响应帧
    vector<uint8_t> respondFrame(5 + sizeof(uint16_t) * (0x0241 - 0x0200 + 1));
    respondFrame[0] = 0x01;
    respondFrame[1] = 0x03;
    respondFrame[2] = 2 + sizeof(uint16_t) * (0x0241 - 0x0200 + 1);
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(respondFrame.data(), 
                                                                respondFrame.size() - sizeof(uint16_t));
        uint8_t *crc16Ptr = respondFrame.data() + respondFrame.size() - sizeof(uint16_t);
        std::memcpy(crc16Ptr, &crc16Modbus, sizeof(uint16_t));
    }

    insertCommunicateInstance(requestFrame, respondFrame);
}

void Simulator::create_bauStatus()
{
    // 创建请求帧
    vector<uint8_t> requestFrame(8);
    requestFrame[0] = 0x01;
    requestFrame[1] = 0x03;
    {
        uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
        *registerAddrPtr = 0x0300;
        miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    }
    {
        uint16_t* registerNumPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
        *registerNumPtr = 0x0352 - 0x0300 + 1;
        miscellaneous::reverseByteArray(registerNumPtr, sizeof(uint16_t));
    }
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestFrame.data(), 6);
        std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    }

    // 创建响应帧
    vector<uint8_t> respondFrame(5 + sizeof(uint16_t) * (0x0352 - 0x0300 + 1));
    respondFrame[0] = 0x01;
    respondFrame[1] = 0x03;
    respondFrame[2] = 2 + sizeof(uint16_t) * (0x0352 - 0x0300 + 1);
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(respondFrame.data(), 
                                                                respondFrame.size() - sizeof(uint16_t));
        uint8_t *crc16Ptr = respondFrame.data() + respondFrame.size() - sizeof(uint16_t);
        std::memcpy(crc16Ptr, &crc16Modbus, sizeof(uint16_t));
    }

    // 修改关键寄存器数值
    {
        // BCU数量
        uint8_t* dataPtr = respondFrame.data() + 3;
        dataPtr += (0x034B - 0x0300) * sizeof(uint16_t);
        uint16_t* realDataPtr = reinterpret_cast<uint16_t*>(dataPtr);
        *realDataPtr = 3;// 3个簇
        miscellaneous::reverseByteArray(realDataPtr, sizeof(uint16_t));
    }
    {
        // BCU在线数量
        uint8_t* dataPtr = respondFrame.data() + 3;
        dataPtr += (0x034C - 0x0300) * sizeof(uint16_t);
        uint16_t* realDataPtr = reinterpret_cast<uint16_t*>(dataPtr);
        *realDataPtr = 3;// 3个簇
        miscellaneous::reverseByteArray(realDataPtr, sizeof(uint16_t));
    }
    {
        // 单个BCU管理的BMU数量
        uint8_t* dataPtr = respondFrame.data() + 3;
        dataPtr += (0x034D - 0x0300) * sizeof(uint16_t);
        uint16_t* realDataPtr = reinterpret_cast<uint16_t*>(dataPtr);
        *realDataPtr = 12;// 12个BMU
        miscellaneous::reverseByteArray(realDataPtr, sizeof(uint16_t));
    }
    {
        // 单个BMU电芯电压数量
        uint8_t* dataPtr = respondFrame.data() + 3;
        dataPtr += (0x034E - 0x0300) * sizeof(uint16_t);
        uint16_t* realDataPtr = reinterpret_cast<uint16_t*>(dataPtr);
        *realDataPtr = 16;// 16个单体电压
        miscellaneous::reverseByteArray(realDataPtr, sizeof(uint16_t));
    }
    {
        // 单个BMU电芯温度数量
        uint8_t* dataPtr = respondFrame.data() + 3;
        dataPtr += (0x034F - 0x0300) * sizeof(uint16_t);
        uint16_t* realDataPtr = reinterpret_cast<uint16_t*>(dataPtr);
        *realDataPtr = 7;// 16个单体温度
        miscellaneous::reverseByteArray(realDataPtr, sizeof(uint16_t));
    }
    {
        // 单个BMU电芯温度数量
        uint8_t* dataPtr = respondFrame.data() + 3;
        dataPtr += (0x0350 - 0x0300) * sizeof(uint16_t);
        uint16_t* realDataPtr = reinterpret_cast<uint16_t*>(dataPtr);
        *realDataPtr = 1;// 1个端子温度
        miscellaneous::reverseByteArray(realDataPtr, sizeof(uint16_t));
    }
    {
        // BCU在线映射
        uint8_t* dataPtr = respondFrame.data() + 3;
        dataPtr += (0x0351 - 0x0300) * sizeof(uint16_t);
        uint32_t* realDataPtr = reinterpret_cast<uint32_t*>(dataPtr);
        *realDataPtr = 1;// 1个BCU
        miscellaneous::reverseByteArray(realDataPtr, sizeof(uint32_t));
    }

    insertCommunicateInstance(requestFrame, respondFrame);
}

void Simulator::create_bauPowerOff()
{
    // 创建请求帧
    vector<uint8_t> requestFrame(8);
    requestFrame[0] = 0x01;
    requestFrame[1] = 0x06;
    {
        uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
        *registerAddrPtr = 0xd700;
        miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    }
    {
        uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
        *registerDataPtr = 0x0000;
        miscellaneous::reverseByteArray(registerDataPtr, sizeof(uint16_t));
    }
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestFrame.data(), 6);
        std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    }

    // 创建响应帧
    vector<uint8_t> respondFrame = requestFrame;// 原样返回
    insertCommunicateInstance(requestFrame, respondFrame);
}

void Simulator::create_bauQuickStartup()
{
    // 创建请求帧
    vector<uint8_t> requestFrame(8);
    requestFrame[0] = 0x01;
    requestFrame[1] = 0x06;
    {
        uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
        *registerAddrPtr = 0xd701;
        miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    }
    {
        uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
        *registerDataPtr = 0x0000;
        miscellaneous::reverseByteArray(registerDataPtr, sizeof(uint16_t));
    }
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestFrame.data(), 6);
        std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    }

    // 创建响应帧
    vector<uint8_t> respondFrame = requestFrame;// 原样返回
    insertCommunicateInstance(requestFrame, respondFrame);
}

void Simulator::create_bcuStatus(const int bcuIndex)
{
    // 创建请求帧
    vector<uint8_t> requestFrame(8);
    requestFrame[0] = 0x01;
    requestFrame[1] = 0x04;

    {
        uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
        *registerAddrPtr = 0x0300 + bcuIndex * 0x0100;
        miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    }
    {
        uint16_t* registerNumPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
        *registerNumPtr = 0x034A - 0x0300 + 1;
        miscellaneous::reverseByteArray(registerNumPtr, sizeof(uint16_t));
    }
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestFrame.data(), 6);
        std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    }

    // 创建响应帧
    vector<uint8_t> respondFrame(5 + sizeof(uint16_t) * (0x034A - 0x0300 + 1));
    respondFrame[0] = 0x01;
    respondFrame[1] = 0x04;
    respondFrame[2] = 2 + sizeof(uint16_t) * (0x034A - 0x0300 + 1);
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(respondFrame.data(), 
                                                                respondFrame.size() - sizeof(uint16_t));
        uint8_t *crc16Ptr = respondFrame.data() + respondFrame.size() - sizeof(uint16_t);
        std::memcpy(crc16Ptr, &crc16Modbus, sizeof(uint16_t));
    }

    insertCommunicateInstance(requestFrame, respondFrame);
}

void Simulator::create_bcuSetRelayGridOff(const int bcuIndex)
{
    // 创建请求帧
    vector<uint8_t> requestFrame(8);
    requestFrame[0] = 0x01;
    requestFrame[1] = 0x06;

    {
        uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
        *registerAddrPtr = 0xd702;
        miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    }
    {
        uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
        *registerDataPtr = bcuIndex;
        miscellaneous::reverseByteArray(registerDataPtr, sizeof(uint16_t));
    }
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestFrame.data(), 6);
        std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    }

    // 创建响应帧
    vector<uint8_t> respondFrame = requestFrame;// 原样返回
    insertCommunicateInstance(requestFrame, respondFrame);
}

void Simulator::create_bcuSetRelayGridOn(const int bcuIndex)
{
    // 创建请求帧
    vector<uint8_t> requestFrame(8);
    requestFrame[0] = 0x01;
    requestFrame[1] = 0x06;

    {
        uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
        *registerAddrPtr = 0xd703;
        miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    }
    {
        uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
        *registerDataPtr = bcuIndex;
        miscellaneous::reverseByteArray(registerDataPtr, sizeof(uint16_t));
    }
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestFrame.data(), 6);
        std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    }

    // 创建响应帧
    vector<uint8_t> respondFrame = requestFrame;// 原样返回
    insertCommunicateInstance(requestFrame, respondFrame);
}

void Simulator::create_bmuCellvolt(const int bcuIndex, const int bmuIndex)
{
    // 创建请求帧
    vector<uint8_t> requestFrame(8);
    requestFrame[0] = 0x01;
    requestFrame[1] = 0x03;

    {
        uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
        *registerAddrPtr = 0x0700 + bcuIndex * 20 * 64 + bmuIndex * 64;
        miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    }
    {
        uint16_t* registerNumPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
        *registerNumPtr = 16;
        miscellaneous::reverseByteArray(registerNumPtr, sizeof(uint16_t));
    }
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestFrame.data(), 6);
        std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    }

    // 创建响应帧
    vector<uint8_t> respondFrame(5 + sizeof(uint16_t) * 16);
    respondFrame[0] = 0x01;
    respondFrame[1] = 0x03;
    respondFrame[2] = 2 + sizeof(uint16_t) * 16;
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(respondFrame.data(), 
                                                                respondFrame.size() - sizeof(uint16_t));
        uint8_t *crc16Ptr = respondFrame.data() + respondFrame.size() - sizeof(uint16_t);
        std::memcpy(crc16Ptr, &crc16Modbus, sizeof(uint16_t));
    }

    insertCommunicateInstance(requestFrame, respondFrame);
}

void Simulator::create_bmuCelltem(const int bcuIndex, const int bmuIndex)
{
    // 创建请求帧
    vector<uint8_t> requestFrame(8);
    requestFrame[0] = 0x01;
    requestFrame[1] = 0x03;

    {
        uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
        *registerAddrPtr = 0x6C00 + bcuIndex * 20 * 66 + bmuIndex * 66;
        miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    }
    {
        uint16_t* registerNumPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
        *registerNumPtr = 64 + 1;
        miscellaneous::reverseByteArray(registerNumPtr, sizeof(uint16_t));
    }
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestFrame.data(), 6);
        std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    }

    // 创建响应帧
    vector<uint8_t> respondFrame(5 + sizeof(uint16_t) * (64 + 1));
    respondFrame[0] = 0x01;
    respondFrame[1] = 0x03;
    respondFrame[2] = 2 + sizeof(uint16_t) * (64 + 1);
    {
        uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(respondFrame.data(), 
                                                                respondFrame.size() - sizeof(uint16_t));
        uint8_t *crc16Ptr = respondFrame.data() + respondFrame.size() - sizeof(uint16_t);
        std::memcpy(crc16Ptr, &crc16Modbus, sizeof(uint16_t));
    }

    insertCommunicateInstance(requestFrame, respondFrame);
}
