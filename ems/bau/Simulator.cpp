#include "Simulator.h"
#include "utils/Miscellaneous.h"
#include "utils/crc16.h"
#include "utils/endian.h"
#include "boost/assert.hpp"
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

    for(int bcuIndex = 0; bcuIndex < 3; ++bcuIndex){
        create_bcuStatus(bcuIndex);
        create_bcuSetRelayGridOff(bcuIndex);
        create_bcuSetRelayGridOn(bcuIndex);

        for(int bmuIndex = 0; bmuIndex < 12; bmuIndex++){
            create_bmuCellvolt(bcuIndex, bmuIndex);
            create_bmuCelltem(bcuIndex, bmuIndex);
        }
    }
}

void Simulator::insertCommunicateInstance(const uint8_t funCode,
        const uint16_t regLeft, const uint16_t regRight, const vector<uint16_t>& regList)
{
    requestRespondMap_.emplace(make_tuple(funCode, regLeft, regRight), regList);
}

vector<uint8_t> Simulator::getRespondFrame(const vector<uint8_t>& request)
{
    const uint8_t devAddr = request[0];
    const uint8_t funCode = request[1];
    uint16_t regAddress{}, regData{};
    ::memcpy(&regAddress, request.data() + 2, sizeof(uint16_t));
    ::memcpy(&regData, request.data() + 4, sizeof(uint16_t));
    endian::reverse(&regAddress, sizeof(uint16_t));
    endian::reverse(&regData, sizeof(uint16_t));

    // 如何功能码是0x06，应该不会类型地查询数据的实际位置

    if(funCode == 0x06){
        // 0x06 对应修改 0x03的数据块
        auto result = std::find_if(requestRespondMap_.begin(), requestRespondMap_.end(),
            [regAddress](pair<const tuple<uint8_t, uint16_t, uint16_t>, vector<uint16_t>>& item){
                auto& [addrRange, regList] = item;
                auto& [frameFunCode, addrLeft, addrRight] = addrRange;
                return (frameFunCode == 0x03) && (regAddress >= addrLeft) && (regAddress <= addrRight);
            }
        );
        BOOST_ASSERT(result != requestRespondMap_.end());
        auto [frameFunCode, addrLeft, addrRight] = result->first;
        auto& frame = result->second;

        endian::reverse(&regData, sizeof(uint16_t));// 以大端形式存储
        frame[regAddress - addrLeft] = regData;
        return request;
    }
    if(funCode == 0x07){
        // 0x07 对应修改 0x04的数据块
        auto result = std::find_if(requestRespondMap_.begin(), requestRespondMap_.end(),
            [regAddress](pair<const tuple<uint8_t, uint16_t, uint16_t>, vector<uint16_t>>& item){
                auto& [addrRange, regList] = item;
                auto& [frameFunCode, addrLeft, addrRight] = addrRange;
                return (frameFunCode == 0x04) && (regAddress >= addrLeft) && (regAddress <= addrRight);
            }
        );
        BOOST_ASSERT(result != requestRespondMap_.end());
        auto [frameFunCode, addrLeft, addrRight] = result->first;
        auto& frame = result->second;

        endian::reverse(&regData, sizeof(uint16_t));// 以大端形式存储
        frame[regAddress - addrLeft] = regData;
        return request;
    }

    // 以下为处理 0x03 或 0x04 功能码 的逻辑。
    auto result = std::find_if(requestRespondMap_.begin(), requestRespondMap_.end(),
        [funCode, regAddress](pair<const tuple<uint8_t, uint16_t, uint16_t>, vector<uint16_t>>& item){
            auto& [addrRange, regList] = item;
            auto& [frameFuCode, addrLeft, addrRight] = addrRange;
            return (funCode == frameFuCode) && (regAddress >= addrLeft) && (regAddress <= addrRight);
        }
    );
    BOOST_ASSERT(result != requestRespondMap_.end());

    auto [frameFunCode, addrLeft, addrRight] = result->first;
    auto& frame = result->second;
    BOOST_ASSERT(frame.size() == regData);

    vector<uint8_t> respond;
    respond.resize(5 + regData * sizeof(uint16_t));
    respond[0] = devAddr;
    respond[1] = funCode;
    respond[2] = regData * sizeof(uint16_t);
    ::memcpy(respond.data() + 3,
                frame.data() + (regAddress - addrLeft),
                regData * sizeof(uint16_t));

    uint16_t crc16Modbus = modbus::crc16_manual(respond.data(), respond.size() - sizeof(uint16_t));
    ::memcpy(respond.data() + respond.size() - sizeof(uint16_t), &crc16Modbus, sizeof(uint16_t));
    return respond;
}

void Simulator::create_bingjiStatus()
{
    // // 创建请求帧
    // vector<uint8_t> requestFrame(8);
    // requestFrame[0] = 0x01;
    // requestFrame[1] = 0x03;
    // {
    //     uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestFrame[2]);
    //     *registerAddrPtr = 0x0200;
    //     endian::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
    // }
    // {
    //     uint16_t* registerNumPtr = reinterpret_cast<uint16_t*>(&requestFrame[4]);
    //     *registerNumPtr = 0x0241 - 0x0200 + 1;
    //     endian::reverseByteArray(registerNumPtr, sizeof(uint16_t));
    // }
    // {
    //     uint16_t crc16Modbus = modbus::crc16_manual(requestFrame.data(), 6);
    //     std::memcpy(&requestFrame[6], &crc16Modbus, sizeof(uint16_t));
    // }

    // // 创建响应帧
    // vector<uint8_t> respondFrame(5 + sizeof(uint16_t) * (0x0241 - 0x0200 + 1));
    // respondFrame[0] = 0x01;
    // respondFrame[1] = 0x03;
    // respondFrame[2] = 2 + sizeof(uint16_t) * (0x0241 - 0x0200 + 1);
    // {
    //     uint16_t crc16Modbus = modbus::crc16_manual(respondFrame.data(), 
    //                                                             respondFrame.size() - sizeof(uint16_t));
    //     uint8_t *crc16Ptr = respondFrame.data() + respondFrame.size() - sizeof(uint16_t);
    //     std::memcpy(crc16Ptr, &crc16Modbus, sizeof(uint16_t));
    // }

    insertCommunicateInstance(0x03, 0x0200, 0x0241, vector<uint16_t>(0x0241 - 0x0200 + 1));
}

void Simulator::create_bauStatus()
{

    vector<uint16_t> frame(0x0352 - 0x0300 + 1);

    // 修改关键寄存器数值
    {
        // 一级保护状态
        bitset<25> test("1111100000000000000000000");
        // bitset<25> test("0000000000000000000000000");
        auto ptr = reinterpret_cast<uint32_t*>(frame.data() + (0x0302 - 0x0300));
        *ptr = static_cast<uint32_t>(test.to_ulong());
        endian::reverse(ptr, sizeof(uint32_t));
    }
    {
        // BCU数量
        auto& reg = frame[0x034B - 0x0300];
        reg = 3;// 3个簇
        endian::reverse(&reg, sizeof(uint16_t));
    }
    {
        // BCU在线数量
        auto& reg = frame[0x034C - 0x0300];
        reg = 3;// 3个簇
        endian::reverse(&reg, sizeof(uint16_t));
    }
    {
        // 单个BCU管理的BMU数量
        auto& reg = frame[0x034D - 0x0300];
        reg = 12;// 12个BMU
        endian::reverse(&reg, sizeof(uint16_t));
    }
    {
        // 单个BMU电芯电压数量
        auto& reg = frame[0x034E - 0x0300];
        reg = 16;// 16个单体电压
        endian::reverse(&reg, sizeof(uint16_t));
    }
    {
        // 单个BMU电芯温度数量
        auto& reg = frame[0x034F - 0x0300];
        reg = 7;// 16个单体温度
        endian::reverse(&reg, sizeof(uint16_t));
    }
    {
        // 单个BMU电芯温度数量
        auto& reg = frame[0x0350 - 0x0300];
        reg = 1;// 1个端子温度
        endian::reverse(&reg, sizeof(uint16_t));
    }
    {
        // BCU在线映射
        // auto& reg = frame[0x0351 - 0x0300];
        auto ptr = reinterpret_cast<uint32_t*>(frame.data() + (0x0351 - 0x0300));
        *ptr = 1;// 1个BCU在线
        endian::reverse(ptr, sizeof(uint32_t));
    }

    insertCommunicateInstance(0x03, 0x0300, 0x0352, frame);
}

void Simulator::create_bauPowerOff()
{
    insertCommunicateInstance(0x03, 0xd700, 0xd700, { 0x00 });
}

void Simulator::create_bauQuickStartup()
{
    insertCommunicateInstance(0x03, 0xd701, 0xd701, { 0x00 });
}

void Simulator::create_bcuStatus(const int bcuIndex)
{
    vector<uint16_t> frame(0x034A - 0x0300 + 1);

    auto ptr = reinterpret_cast<uint32_t*>(frame.data() + 0x0E);
    bitset<5> data("10001");
    *ptr = static_cast<uint32_t>(data.to_ulong());
    endian::reverse(ptr, sizeof(uint32_t));

    insertCommunicateInstance(0x04,
        0x0300 + bcuIndex * 0x0100,
        0x0300 + bcuIndex * 0x0100 + (0x034A - 0x0300 + 1),
        frame);
}

void Simulator::create_bcuSetRelayGridOff(const int bcuIndex)
{
    insertCommunicateInstance(0x03, 0xd702, 0xd702, { 0x00 });
}

void Simulator::create_bcuSetRelayGridOn(const int bcuIndex)
{
    const uint16_t regData = bcuIndex;
    insertCommunicateInstance(0x03, 0xd703, 0xd703, { regData });
}

void Simulator::create_bmuCellvolt(const int bcuIndex, const int bmuIndex)
{
    if(bcuIndex == 0 && bmuIndex == 2){
        uint16_t test = 123;
        endian::reverse(&test, sizeof(uint16_t));
        vector<uint16_t> frameTest(16, test);

        insertCommunicateInstance(0x03,
            0x0700 + bcuIndex * 20 * 64 + bmuIndex * 64,
            0x0700 + bcuIndex * 20 * 64 + bmuIndex * 64 + 16,
            frameTest);
        return;
    }
    vector<uint16_t> frame(16);
    insertCommunicateInstance(0x03,
        0x0700 + bcuIndex * 20 * 64 + bmuIndex * 64,
        0x0700 + bcuIndex * 20 * 64 + bmuIndex * 64 + 16,
        frame);
}

void Simulator::create_bmuCelltem(const int bcuIndex, const int bmuIndex)
{
    if(bcuIndex == 0 && bmuIndex == 3){
        uint16_t test = 2730 + 4560;
        endian::reverse(&test, sizeof(uint16_t));

        vector<uint16_t> frame(64 + 1, test);
        insertCommunicateInstance(0x03,
            0x6C00 + bcuIndex * 20 * 66 + bmuIndex * 66,
            0x6C00 + bcuIndex * 20 * 66 + bmuIndex * 66 + (64 + 1),
            frame);
    }
    vector<uint16_t> frame(64 + 1);
    insertCommunicateInstance(0x03,
        0x6C00 + bcuIndex * 20 * 66 + bmuIndex * 66,
        0x6C00 + bcuIndex * 20 * 66 + bmuIndex * 66 + (64 + 1),
        frame);
}
