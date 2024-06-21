#include "ems/profit_strategy/TypeList.h"
#include <iostream>

using namespace std;

int main()
{
    // // 创建数据库表
    // const std::string dbname = ems::electricity_price::typelist::createTable();
    // std::cout << "dbname: " << dbname << std::endl;

    // // 插入默认记录
    // // const bool insertResult = electorcity_price_typelist::insertIntoDefaultRecord();
    // vector<string> values{ "price1",
    //         "1.400", "1.400",
    //         "1.300", "1.300",
    //         "1.200", "1.200",
    //         "1.100", "1.100"
    //         };
    // auto insertResult = ems::electricity_price::typelist::insertRecord(values[0],
    //                                                          values[1], values[2],
    //                                                          values[3], values[4],
    //                                                          values[5], values[6],
    //                                                          values[7], values[8]);
    // if(insertResult.has_value())
    //     std::cout << "insertResult: " << insertResult.value() << std::endl;

    // // 查询指定记录
    // const auto record = ems::electricity_price::typelist::getRecord("price1");
    // if(record.has_value()){
    //     auto value = record.value();
    //     std::cout << std::get<0>(value)
    //                     << '|' << std::get<1>(value)
    //                     << '|' << std::get<2>(value)
    //                     << '|' << std::get<3>(value)
    //                     << '|' << std::get<4>(value)
    //                     << '|' << std::get<5>(value)
    //                     << '|' << std::get<6>(value)
    //                     << '|' << std::get<7>(value);
    // }
}
