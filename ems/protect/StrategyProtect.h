#pragma once
#include <thread>
#include <memory>
#include "utils/Log4cppWrapper.h"
#include "msgpack.hpp"
#include "utils/ZmqNode.h"
#include "ems/bau/DeviceStatus.h"
#include "ems/station/Model.h"

/**
 * BAU状态 & 并机状态
 * 
 * 
*/

namespace ems
{
namespace xftg
{
using namespace std;

class StrategyProtect
{
public:
    StrategyProtect(std::shared_ptr<zmq::socket_t> stationDealer);
    ~StrategyProtect();
    void start();
private:
    void doWork();
    void control(const StationInfo& stationInfo);
    bool doBauUpdateInfo(const bau::BauInfo& bauInfo);
    bool doPcsUpdateInfo(const pcs::PcsInfo& pcsInfo);
    
    vector<uint8_t> getFramePowerOn();
    vector<uint8_t> getFramePowerOff();

    log4cpp::Category& log_;
    std::shared_ptr<zmq::socket_t> stationDealer_;
    bau::DeviceStatus bauStatus_;
    std::thread loopThread_;
};
}//namespace xftg
}//namespace ems
