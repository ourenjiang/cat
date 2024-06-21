#include "ems/xftg_strategy/ExecutePlan.h"
#include <iostream>

int main()
{
    // 创建数据库表
    const std::string dbname = xftg_execute::createTable();
    std::cout << "dbname: " << dbname << std::endl;

    // 插入默认记录
    const bool insertResult = xftg_execute::insertIntoDefaultRecord();
    std::cout << "insertResult: " << insertResult << std::endl;

    // 插入指定记录
    const bool insertResult2 = xftg_execute::insertRecord("xftg_execute2",
                                                        "duration1",
                                                        "12:00", "13:00",
                                                        "discharge", "5kW");
    std::cout << "insertResult2: " << insertResult2 << std::endl;

    // 删除指定记录
    const bool deleteResult = xftg_execute::deleteRecord("xftg_execute2", "duration1");
    std::cout << "deleteResult: " << deleteResult << std::endl;

    // 修改指定记录
    const bool modifyResult = xftg_execute::modifyRecord("xftg_execute1", "duration1",
                                                        "07:00", "14:00",
                                                        "charge", "12kW");
    std::cout << "modifyResult: " << modifyResult << std::endl;

    // 查询指定记录
    std::cout << xftg_execute::getRecord("xftg_execute1", "duration1") << std::endl;
    std::cout << std::endl;
    std::cout << xftg_execute::getRecord("xftg_execute1") << std::endl;
    std::cout << std::endl;
    std::cout << xftg_execute::getRecordNameList() << std::endl;
}
