#include <iostream>

using namespace std;

struct A{
    ~A() { throw 1; }
};

struct B{
    ~B() noexcept(false) { throw 1; }
};

int main()
{
    try{
        A a;
        B b;
    }
    catch(...)
    {
        cout << "B is except" << endl;
    }
}