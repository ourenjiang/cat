#include "utils/BoostTimer.h"
#include <iostream>

using namespace std;

class MyTest
{
public:
    int run(int params)
    {
        cout << "boost timer... " << params << endl;
        return 0;
    }
};

int main()
{
    // ems::BoostTimer timer([](){
    //     cout << "boost timer..." << endl;
    // }, 1000);

    MyTest test;
    ems::BoostTimer timer(std::bind(&MyTest::run, &test, 1), 1000);
    timer.start();
}