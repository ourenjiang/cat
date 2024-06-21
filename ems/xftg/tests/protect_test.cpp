#include "ems/xftg_strategy/ProtectParams.h"
#include <iostream>

int main()
{
    // 创建数据库表
    const std::string dbname = xftg_protect::createTable();
    std::cout << "dbname: " << dbname << std::endl;

    // 插入默认记录
    const bool insertResult = xftg_protect::insertIntoDefaultRecord();
    std::cout << "insertResult: " << insertResult << std::endl;

    // 插入指定记录
    const bool insertResult2 = xftg_protect::insertRecord("xftg_protect2",
                                                        "20%", "15%",
                                                        "100kW", "10kW",
                                                        "5kW", "4kW");
    std::cout << "insertResult2: " << insertResult2 << std::endl;

    // 删除指定记录
    const bool deleteResult = xftg_protect::deleteRecord("xftg_protect1");
    std::cout << "deleteResult: " << deleteResult << std::endl;

    // 修改指定记录
    const bool modifyResult = xftg_protect::modifyRecord("xftg_protect2",
                                                        "25%", "16%",
                                                        "110kW", "18kW",
                                                        "6kW", "40kW");
    std::cout << "modifyResult: " << modifyResult << std::endl;

    // 查询指定记录
    std::cout << xftg_protect::getRecord("xftg_protect2") << std::endl;
    std::cout << std::endl;
    std::cout << xftg_protect::getRecordNameList() << std::endl;
}
