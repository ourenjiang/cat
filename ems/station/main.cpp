#include "utils/ResourceInit.h"
#include "ems/station/Station.h"
#include "utils/SyncSocketRespond.h"
#include "ems/pcs/Simulator.h"
#include "ems/bau/Simulator.h"
using namespace std;
using namespace ems;

int main()
{
    {
        /**
         * 为了方便测试，这里手动添加环境变量；
         * 实际生产环境部署时应该移除.
        */
        const string envName{ "PACEIC_EMS_SERVER" };
        const string envValue{ "/opt/paceic_ems_server/main" };
        ::setenv(envName.c_str(), envValue.c_str(), 0);
    }

    utils::configurationInitFromEnv();//初始化配置文件
    utils::logInit();// 初始化日志模块

    // pcs::Simulator 
    auto pcsSimulator = make_unique<pcs::Simulator>();
    // SyncSocketRespond pcsSyncRespond("9001",
    SyncSocketRespond pcsSyncRespond("8001",
                                    std::bind(&pcs::Simulator::getRespondFrame,
                                                                pcsSimulator.get(),
                                                                std::placeholders::_1));
    // bau::Simulator 
    auto bauSimulator = make_unique<bau::Simulator>();
    // SyncSocketRespond bauSyncRespond("1502",
    SyncSocketRespond bauSyncRespond("2502",
                                    std::bind(&bau::Simulator::getRespondFrame,
                                                                bauSimulator.get(),
                                                                std::placeholders::_1));

    Station station;
    station.start();
}
