#include <iostream>

struct Object // bad1: 缺少数据成员初始化
{
    int index;
    int id;
};

struct World
{
    Object objects[2]{};
    Object *positions[2]{}; 
};

void next_level(World& world, Object& object)
{
    world.positions[object.index] = &object;// object.index 未初始化数据的引用
}

void do_bad(World& world, Object object)
{
    object.id = 42;
    next_level(world, object);//随着do_bad()调用栈的退出，next_level() 中 world.positions的内容将空悬
}

int main(int argc, char** /*argv*/)
{
    World world{};
    auto index = (argc == 1) ? 0 : 1;// bad2: 参数映射逻辑反直觉，1 => 1 ?
    do_bad(world, world.objects[index]);
    std::cout << "Hi\n";
    std::cout << world.positions[0]->id << '\n';// 打印的是未初始化的随机值
    /**
     * 实际测试：
     * - 在gcc13中，开启-Wall参数也未提示任何警告；
     * - 输出：42，与命令行入参无关。
    */
}
