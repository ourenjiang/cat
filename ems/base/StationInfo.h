#pragma once
#include "ems/station/Model.h"
#include "zmq.hpp"

namespace ems
{
namespace base
{

StationInfo getStationInfo(shared_ptr<zmq::socket_t> stationDealer);

}//namespace base
}//namespace ems
