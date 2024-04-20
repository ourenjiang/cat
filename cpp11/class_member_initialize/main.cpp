#include <iostream>

using namespace std;

struct A{
    ~A() { throw 1; }
    int c = 1;
    // static int b = 1;
};

int main()
{

}