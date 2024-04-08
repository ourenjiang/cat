#include "RedisWrapper.h"
using namespace redis_wrapper;

RedisMap Object::redisMap_;

void Object::init(initializer_list<string> _list, const string& _addr)
{
    for(auto itr = _list.begin(); itr != _list.end(); ++itr)
    {
        auto index = std::distance(_list.begin(), itr);
        redisMap_.emplace(static_cast<RedisId>(index), Redis(_addr));
    }
}
