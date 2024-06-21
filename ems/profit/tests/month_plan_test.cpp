#include "ems/profit_strategy/MonthPlan.h"
#include <iostream>

int main()
{
    // // 创建数据库表
    // const std::string dbname = ems::electricity_price::monthplan::createTable();
    // std::cout << "dbname: " << dbname << std::endl;

    // // 插入默认记录
    // const bool insertResult = ems::electricity_price::monthplan::insertIntoDefaultRecord();
    // std::cout << "insertResult: " << insertResult << std::endl;

    // // 查询指定记录
    // const auto optValue = ems::electricity_price::monthplan::getRecord("electricity_price::monthplan1");
    // if(optValue.has_value()){
    //     const auto [monthNo, durationName, priceName] = optValue.value();
    //     std::cout << monthNo << '|' << durationName << '|' << priceName << std::endl;
    // }
}
