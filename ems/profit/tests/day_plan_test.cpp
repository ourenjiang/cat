#include "ems/profit_strategy/DayPlan.h"
#include <iostream>

int main()
{
    // // 创建数据库表
    // const std::string dbname = ems::electricity_price::dayplan::createTable();
    // std::cout << "dbname: " << dbname << std::endl;

    // // 插入默认记录
    // const bool insertResult = ems::electricity_price::dayplan::insertIntoDefaultRecord();
    // std::cout << "insertResult: " << insertResult << std::endl;

    // // 插入指定记录
    // const bool insertResult2 = ems::electricity_price::dayplan::insertRecord("electricity_price::dayplan1",
    //                                                                 "duration1", "feng",
    //                                                                 "12:00", "13:00");
    // std::cout << "insertResult2: " << insertResult2 << std::endl;

    // // 删除指定记录
    // const bool deleteResult = ems::electricity_price::dayplan::deleteRecord("day1");
    // std::cout << "deleteResult: " << deleteResult << std::endl;

    // // 修改指定记录
    // const bool modifyResult = ems::electricity_price::dayplan::modifyRecord("electricity_price::dayplan1",
    //                                                     "duration1", "ping",
    //                                                     "14:00", "16:00");
    // std::cout << "modifyResult: " << modifyResult << std::endl;

    // // 查询指定记录
    // std::cout << std::endl;
    // const auto records = ems::electricity_price::dayplan::getRecord("electricity_price::dayplan1");
    // if(records.has_value()){
    //     for(const auto& item: records.value()){
    //         auto [durationName, durationType, durationBegin, durationEnd] = item;
    //         std::cout << durationName << '|' << durationType << '|' << durationBegin << '|' << durationEnd << std::endl;
    //     }
    // }

    // // 查询名称列表
    // auto optValue = ems::electricity_price::dayplan::getRecordNameList();
    // if(optValue.has_value()){
    //     for(const std::string& item: optValue.value()){
    //         std::cout << item << std::endl;
    //     }
    // }
}
