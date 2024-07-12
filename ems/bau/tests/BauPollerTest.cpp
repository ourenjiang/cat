#include "ems/bau/BauPoller.h"
#include "ems/bau/BauRouter.h"
#include "utils/SyncRespond.h"
#include "ems/bau/Simulator.h"
#include "utils/ResourceInit.h"
#include <iostream>

using namespace std;
using namespace ems;
using namespace ems::bau;

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
    // bau::Simulator 
    auto bauSimulator = make_unique<bau::Simulator>();
    // SyncRespond bauSyncRespond("1502",
    SyncRespond bauSyncRespond("2502",
                                    std::bind(&bau::Simulator::getRespondFrame,
                                                                bauSimulator.get(),
                                                                std::placeholders::_1));


    // PollRouter router = createPollRouter("tcp://127.0.0.1:12000", "127.0.0.1:2502");
    // router.start();
    // BauPoller poller("tcp://127.0.0.1:12000", 1000, "BauSummary", "tcp://127.0.0.1:12001");
    // poller.start();
}
