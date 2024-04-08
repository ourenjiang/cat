#include "cat/utils/RedisWrapper.h"
#include <iostream>

int main()
{
    redis_wrapper::Object::init({ "conn1", "conn2"}, "tcp://192.168.10.251:6379");
    auto& conn1 = redis_wrapper::Object::getRedis(redis_wrapper::RedisId::Redis_1);
    auto& conn2 = redis_wrapper::Object::getRedis(redis_wrapper::RedisId::Redis_2);

    auto result = conn1.hget("ems:system_status:running_duration", "runTimeDesc");
    std::cout << result.value() << std::endl;
    getchar();
}
