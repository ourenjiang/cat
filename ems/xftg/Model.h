#pragma once
#include "msgpack.hpp"

namespace ems
{
namespace xftg
{
using namespace std;

struct StrategyInfo
{
    bool autoRun;
    string activeStrategy;
    MSGPACK_DEFINE(autoRun, activeStrategy)
};

}//namespace xftg
}//namespace ems
