#pragma once
#include "Model.h"
#include "zmq.hpp"
#include "utils/SyncRequest.h"
#include <bitset>

namespace ems
{
namespace pcs
{
using namespace std;

class PcsCollector
{
public:
    PcsCollector(std::shared_ptr<zmq::socket_t> stationDealer, std::shared_ptr<SyncRequest> modbusProxy);
    ~PcsCollector();

    void start();
private:
    void doWork();

    void doCommand(zmq::message_t& srcIdentity);

    void doPoll_0406_0460();
    _0406_0460_Summary createSummary_0406_0460(const vector<uint16_t>&);
    void doPoll_0474_04D0();
    _0474_04D0_Summary createSummary_0474_04D0(const vector<uint16_t>&);

    std::optional<vector<byte>> sendAndRecv(const vector<uint8_t>& msg);
    void handlePcsFrame();

    WarningStatus calcWarning(const bitset<16> bits1, const bitset<16> bits2);
    int calcWarningCount(const WarningStatus status);
    FaultStatus calcFault(const bitset<16> bits1, const bitset<16> bits2);
    int calcFaultCount(const FaultStatus status);
    RunStatus calcRun(const uint16_t bits1, const uint16_t bits2, const uint16_t bits3);

    PcsInfo pcsInfo_;
    std::shared_ptr<zmq::socket_t> stationDealer_;
    std::shared_ptr<SyncRequest> modbusProxy_;
    std::thread loopThread_;
};
}//namespace pcs
}//namespace ems
