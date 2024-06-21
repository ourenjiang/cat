#include "ems/xftg_strategy/Interface.h"
#include <iostream>

int main()
{
    // 创建数据库表
    const std::string dbname = xftg_strategy::createTable();
    std::cout << "dbname: " << dbname << std::endl;

    // 插入默认记录
    const bool insertResult = xftg_strategy::insertIntoDefaultRecord();
    std::cout << "insertResult: " << insertResult << std::endl;

    // 插入指定记录
    const bool insertResult2 = xftg_strategy::insertRecord("xftg_strategy2",
                                                        "execute2", "protect2",
                                                        "[true, false, true, false, true, false, true]",
                                                        "2024-07-15", "2024-08-15",
                                                        "level2", "branch2");
    std::cout << "insertResult2: " << insertResult2 << std::endl;

    // 删除指定记录
    const bool deleteResult = xftg_strategy::deleteRecord("xftg_strategy1");
    std::cout << "deleteResult: " << deleteResult << std::endl;

    // 修改指定记录
    const bool modifyResult = xftg_strategy::modifyRecord("xftg_strategy2",
                                                        "execute3", "protect3",
                                                        "[false, false, true, false, true, false, true]",
                                                        "2024-09-15", "2024-10-15",
                                                        "level3", "branch3");
    std::cout << "modifyResult: " << modifyResult << std::endl;

    // 查询指定记录
    std::cout << xftg_strategy::getRecord("xftg_strategy2") << std::endl;
    std::cout << std::endl;
    std::cout << xftg_strategy::getRecordNameList() << std::endl;
}
