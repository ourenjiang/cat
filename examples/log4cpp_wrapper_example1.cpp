#include "cat/utils/Log4cppWrapper.h"

int main()
{
    log4cpp_wrapper::Object::init({
        { "test1", "DEBUG" },
        { "test2", "INFO" },
    }, "./");

    auto& logger1 = log4cpp_wrapper::Object::getLogger(log4cpp_wrapper::LogId::Logger_1);
    logger1.debug("hello world");
    auto& logger2 = log4cpp_wrapper::Object::getLogger(log4cpp_wrapper::LogId::Logger_2);
    logger2.debug("hello world2");
}
