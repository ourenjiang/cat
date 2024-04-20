#include <iostream>
using namespace std;

class Helloworld
{
public:
    // void func(int value)
    // {
    //     cout << "call: " << __PRETTY_FUNCTION__ << endl;
    // }

    void func(int&& value)
    {
        cout << "call: " << __PRETTY_FUNCTION__ << endl;
    }
};

int main()
{
    Helloworld helloworld;
    int&& value = 42;
    cout << &value << endl;
    helloworld.func(std::move(value));
    // helloworld.func(42);
    helloworld.func(1);
}
