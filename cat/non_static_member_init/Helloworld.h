#pragma once

/**
 * 特性描述：使用inline修饰非常量静态成员
 * 比较：与未修饰的非常量静态成员相比，使得其声明和初始化(定义)可以一次性完成，代码更简洁;
 * 比较：与常量静态成员相比，保留了非常量的灵活性
*/

class Helloworld
{
public:
    Helloworld();
private:
    static int member1_;// right 只能声明，定义必须放在类外；并且，如果尝试将其放在头文件中以达到数据的整体性，则会因为头文件引入而出现重复定义的链接错误，
    static const int member2_{ 42 };// right 常量支持声明与定义同时进行
    inline static int member3_{ 16 };// right 保证了声明与初始化的完整性，同时又保留了非常量的灵活性
};

// int Helloworld::member1_{ 12 }; // 会引发重复定义链接错误，所以只能以分离编译的方式将其定义在源文件而不是头文件中
