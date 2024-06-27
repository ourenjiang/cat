#pragma once
#include <iostream>
#include "msgpack.hpp"

namespace msgpackWrapper
{

/**
 * 注意，这里我尝试将返回类型声明为&&，但编译器警告返回局部变量。
*/
template <typename T>
inline msgpack::sbuffer pack(T&& v)
{
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, std::forward<T>(v));
    return sbuf;
}

template <typename T>
inline bool unpack(const void* data, size_t len, T&& unpackedMsg)
{
    msgpack::object_handle oh;
    try{
        oh = msgpack::unpack(static_cast<const char*>(data), len);
    }
    catch(msgpack::v1::insufficient_bytes& e){
        std::cout << "unpack deserialize failed: " << e.what() << std::endl;
        return false;
    }
    catch(...){
        std::cout << "unpack deserialize, but unexpected" << std::endl;
        return false;
    }

    try{
        oh.get().convert(std::forward<T>(unpackedMsg));
    }
    catch(msgpack::v1::type_error& e){
        std::cout << "unpack deserialize failed: " << e.what() << std::endl;
        return false;
    }
    catch(...){
        std::cout << "unpack deserialize failed, but unexpected" << std::endl;
        return false;
    }
    return true;
}

}//namespace _msgpack
