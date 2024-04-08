#pragma once
#include "sw/redis++/redis++.h"
#include <initializer_list>

namespace redis_wrapper
{
using namespace std;
using namespace sw::redis;

enum class RedisId
{
    Redis_1,
    Redis_2
};

using RedisMap = map<RedisId, Redis>;

class Object
{
public:
    Object() = delete;
    static void init(initializer_list<string> _list, const string& _addr);
    static Redis& getRedis(RedisId id){ return redisMap_.at(id); }
private:
    static RedisMap redisMap_;
};

}//namespace redis_wrapper
