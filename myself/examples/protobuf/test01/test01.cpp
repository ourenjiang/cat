#include <iostream>
#include <type_traits>
#include "Person.pb.h"  // 请替换成实际的 Protobuf 消息头文件

int main() {
    // 替换成你的 Protobuf 消息类型
    using MessageType = tutorial::Persion;

    // 使用 std::is_pod 检查是否为 POD 类型
    if (std::is_pod<MessageType>::value) {
        std::cout << "The Protobuf message is a POD type." << std::endl;
    } else {
        std::cout << "The Protobuf message is not a POD type." << std::endl;
    }

    return 0;
}
