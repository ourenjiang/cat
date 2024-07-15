#include "StationInfo.h"
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"

using namespace ems;
using namespace ems::base;

StationInfo ems::base::getStationInfo(shared_ptr<zmq::socket_t> dataDealer)
{
    dataDealer->send(zmq::message_t(string("ReadInfo")), zmq::send_flags::none);
    zmq::message_t rcvmsg;
    (void)dataDealer->recv(rcvmsg);

    StationInfo stationInfo;
    const bool unpackResult = msgpackWrapper::unpack(rcvmsg.data(), rcvmsg.size(), stationInfo);
    BOOST_ASSERT(unpackResult);
    return stationInfo;
}
