#include "Helloworld.h"
#include <iostream>
using namespace std;

int Helloworld::member1_{ 12 };

Helloworld::Helloworld()
{
    cout << "member1_: " << member1_ << endl;
    cout << "member2_: " << member2_ << endl;
    cout << "member3_: " << member3_ << endl;
}