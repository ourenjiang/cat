#include "Controler.h"
#include "utils/YamlcppWrapper.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "boost/assert.hpp"
#include "utils/Miscellaneous.h"

using namespace std;
using namespace ems::pcs;

Controler::Controler()
    : log_(ems::Log4cppWrapper::getLogger(0))
{
    {
        auto& cfgRoot = YamlcppWrapper::getRoot();
        const auto& collectors = cfgRoot["collector"];
        auto resultLoad = std::find_if(collectors.begin(), collectors.end(), [](const YAML::Node& item){
            return item["name"].as<string>() == "PCS";
        });
        BOOST_ASSERT(resultLoad != collectors.end());
        BOOST_ASSERT((*resultLoad)["load"].as<bool>());

        const string proxyAddress = (*resultLoad)["master"]["address"].as<string>();
        requester_ = make_unique<ZmqRequest>(proxyAddress);
    }
}

bool Controler::pollMessage(vector<uint8_t> &requestMessage, vector<uint8_t> &respondMessage)
{
    auto serializedMsg = msgpackWrapper::pack(requestMessage);// 序列化
    const vector<byte> sendmsg(reinterpret_cast<byte*>(serializedMsg.data()),
                                reinterpret_cast<byte*>(serializedMsg.data()) + serializedMsg.size());
    bool sendResult = requester_->send(sendmsg);
    if(!sendResult)
    {
        log_.error("send failed");
    }

    // 接收
    const auto recvResult = requester_->recv();
    if(!recvResult.has_value()){
        cout << "recv failed" << endl;
        return false;
    }
    const auto recvmsg = recvResult.value();

    // 2.2 消息反序列化
    if(!msgpackWrapper::unpack(recvmsg.data(), recvmsg.size(), respondMessage))
    {
        return false;
    }
    log_.debug("recv success");
    return true;
}

void Controler::setPowerOff()
{
    vector<uint8_t> requestMessage(8);
    {
        requestMessage[0] = 0x01;
        requestMessage[1] = 0x06;
        {
            uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestMessage[2]);
            *registerAddrPtr = 0x04CC;//开关机方式
            miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
        }
        {
            uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestMessage[4]);
            *registerDataPtr = 0xAAAA;// 关机
            miscellaneous::reverseByteArray(registerDataPtr, sizeof(uint16_t));
        }
        {
            uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestMessage.data(), 6);
            std::memcpy(&requestMessage[6], &crc16Modbus, sizeof(uint16_t));
        }
    }

    vector<uint8_t> respondMessage;
    if(pollMessage(requestMessage, respondMessage))
    {
        log_.debug("set pcs off success");
    }
}

void Controler::setPowerOn()
{
    vector<uint8_t> requestMessage(8);
    {
        requestMessage[0] = 0x01;
        requestMessage[1] = 0x06;
        {
            uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestMessage[2]);
            *registerAddrPtr = 0x04CC;//开关机方式
            miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
        }
        {
            uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestMessage[4]);
            *registerDataPtr = 0x5555;// 开机
            miscellaneous::reverseByteArray(registerDataPtr, sizeof(uint16_t));
        }
        {
            uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestMessage.data(), 6);
            std::memcpy(&requestMessage[6], &crc16Modbus, sizeof(uint16_t));
        }
    }

    vector<uint8_t> respondMessage;
    if(pollMessage(requestMessage, respondMessage))
    {
        log_.debug("set pcs off success");
    }
}

void Controler::setActivePower(const string& status, const double power)
{
    // saveNewPowerHistory(userCheckedAction.activePower);
    // log_.debugStream() << "|======================================================2";
    BOOST_ASSERT(power >= 0 && power <= 50.0);
    BOOST_ASSERT(status == "charge" || status == "discharge");

    uint16_t activePower = static_cast<uint16_t>(power * 0.1);//单位:0.1kW
    if(status == "charge") activePower *= (-1);

    vector<uint8_t> requestMessage(8);
    {
        requestMessage[0] = 0x01;
        requestMessage[1] = 0x06;
        {
            uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestMessage[2]);
            *registerAddrPtr = 0x04C4;
            miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
        }
        {
            uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestMessage[4]);
            *registerDataPtr = activePower;
            miscellaneous::reverseByteArray(registerDataPtr, sizeof(uint16_t));
        }
        {
            uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestMessage.data(), 6);
            std::memcpy(&requestMessage[6], &crc16Modbus, sizeof(uint16_t));
        }
    }

    vector<uint8_t> respondMessage;
    if(!pollMessage(requestMessage, respondMessage))
    {
        return;
    }
    // log_.debug("strategy command success");
}

void Controler::verifyRemoteMode()
{
    if(needSetRemoteMode())
    {
        setRemoteMode();
    }
}

bool Controler::verifyFault()
{
    if(needOffByPcsFault())
    {
        setPowerOff();
        return true;
    }
    return false;
}

void Controler::verifyPowerOn()
{
    if(needOnByPcsValidPower())
    {
        setPowerOn();
    }
}

double Controler::getCurrentSettingPower()
{
    // const string key{ "pcs_0:base" };
    // const string field{ "output_activepower_setting" };
    // BOOST_ASSERT(redis_->hexists(key, field));
    // auto result = redis_->hget(key, field);
    // // log_.debugStream() << field << " => 输出有功功率设定值(kW): " << result.value();
    // return stod(result.value());
    return 0.0;
}

void Controler::setRemoteMode()
{
    //定义请求消息
    vector<uint8_t> requestMessage(8);
    {
        requestMessage[0] = 0x01;
        requestMessage[1] = 0x06;
        {
            uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestMessage[2]);
            *registerAddrPtr = 0x04CD;//调度方式
            miscellaneous::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
        }
        {
            uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestMessage[4]);
            *registerDataPtr = 1;
            miscellaneous::reverseByteArray(registerDataPtr, sizeof(uint16_t));
        }
        {
            uint16_t crc16Modbus = miscellaneous::getCrc16Modbus(requestMessage.data(), 6);
            std::memcpy(&requestMessage[6], &crc16Modbus, sizeof(uint16_t));
        }
    }

    vector<uint8_t> respondMessage;
    if(pollMessage(requestMessage, respondMessage))
    {
        log_.debug("set remote mode success");
    }
}

bool Controler::needOffByPcsFault()
{
    // const string key { "pcs_0:warn_status3" };
    // const string field{ "blqgzz" };
    // BOOST_ASSERT(redis_->hexists(key, field));
    // auto result = redis_->hget(key, field);
    // if(result.value() == "1")
    // {
    //     log_.debugStream() << field << " => 变流器故障总: " << result.value();
    //     return true;
    // }
    return false;
}

bool Controler::needOnByPcsValidPower()
{
    // uint16_t blqkjztz{ 0 };// 变流器开关机状态总
    // {
    //     string key { "pcs_0:warn_status3" };
    //     string field{ "blqkjztz" };
    //     auto result = redis_->hget(key, field);
    //     if(result.has_value())
    //     {
    //         log_.debugStream() << field << " => 变流器开关机状态总: " << result.value();
    //         blqkjztz = stoul(result.value());
    //         if(blqkjztz == 1)
    //         {
    //             return false;// 已经是开机状态了。
    //         }
    //     }
    // }

    // // 对'有功功率设置值'进行非零检查
    // // 连续10次出现非零值，代表'需要开机'。
    // static double checkValidCount{ 0 };
    // const double currentPower = getCurrentSettingPower();
    // if(currentPower != 0)
    // {
    //     ++checkValidCount;
    //     if(checkValidCount > 10)
    //     {
    //         checkValidCount = 0;//重置清零
    //         return true;
    //     }
    // }
    // else
    // {
    //     checkValidCount = 0;//重置清零
    // }
    return false;
}

bool Controler::needSetRemoteMode()
{
    // const string key{ "pcs_0:base" };
    // const string field{ "diaodu_mode" };
    // BOOST_ASSERT(redis_->hexists(key, field));
    // auto result = redis_->hget(key, field);
    // if(result.value() == "0")
    // {
    //     log_.debugStream() << field << " => 调度方式(0本地, 1远程): " << result.value();
    //     return true;
    // }
    return false;
}
