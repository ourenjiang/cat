#pragma once
#include "utils/ZmqRequest.h"
#include "utils/Log4cppWrapper.h"

namespace ems
{
namespace pcs
{
using namespace std;

class Controler
{
public:
    Controler();
    // bool pollMessage(vector<uint8_t>& requestMessage, vector<uint8_t>& respondMessage);
    std::optional<vector<uint8_t>> pollMessage(const vector<uint8_t>& reqmsg);
    void setActivePower(const string& status, const double power);// 设置有功功率
    void verifyRemoteMode();
    bool verifyFault();
    void verifyPowerOn();
    double getCurrentSettingPower();
    void setPowerOff();// 关机
    void setPowerOn();// 开机
private:
    bool needSetRemoteMode();  // 检'调度方式'
    void setRemoteMode();  // 设置'调度方式'为远程
    bool needOffByPcsFault();// 因PCS故障而关闭策略
    bool needOnByPcsValidPower();// 因长期有效的功率值而开关

    log4cpp::Category& log_;
    zmq::socket_t zmqDealer_;
};
}//namespace pcs
}//namespace ems
