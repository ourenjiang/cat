#include "HistoryWarning.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "sqlite_modern_cpp.h"

using namespace ems::base;

void HistoryWarning::insertRecord(const string content, const string level, const string deviceName, const string createTime, const string action, const string processed)
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Warning.sqlite" };
        sqlite::database WarningDb(filename);
        WarningDb << "INSERT INTO WARN ("
                        "CONTENT, "         // 告警内容
                        "LEVEL, "           // 告警级别
                        "DEVICE_NAME, "     // 所属设备名称
                        "CREATE_TIME, "     // 记录产生时间
                        "ACTION, "           // 动作
                        "PROCESSED) "       // 处理状态
                        "VALUES (?, ?, ?, ?, ?, ?);"
                    << content << level << deviceName << createTime << action << processed;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
}
