#include "ems/station/StationBuilder.h"
#include "utils/ResourceInit.h"
#include "utils/SyncRespond.h"
#include "ems/bau/Simulator.h"
#include <iostream>

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
    // bau::Simulator 
    auto bauSimulator = make_unique<bau::Simulator>();
    // SyncRespond bauSyncRespond("1502",
    SyncRespond bauSyncRespond("2502",
                                    std::bind(&bau::Simulator::getRespondFrame,
                                                                bauSimulator.get(),
                                                                std::placeholders::_1));

    StationBuilder builder;
    {
        const string port("10001");
        builder.prebuildZmqNode(port);
    }

    {
        bau::BauCollectorInitParams bauCollectorParams{
            .pollIntervalMs = 1000,
            .slaveAddress = "127.0.0.1:2502",
            .pollRouterPort = "12000",
            .identityForBranch = "BauDealerForBranch"
        };

        BranchInitParams params{
            .zmqPort = "10002",
            .identity = "Branch_0",
            .index = 0,
            .bauCollectorInitParams = bauCollectorParams
        };
        builder.addBranch(params);
    }
    auto station = builder.build();
    station->start();
}
