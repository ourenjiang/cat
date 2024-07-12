#pragma once
#include <bitset>

namespace ems
{
namespace base
{
using namespace std;


class BauWarning
{
public:
    static int calcUniformLevel(const int bitIndex,
        const bitset<32> level1Bitset, const bitset<32> level2Bitset, const bitset<32> level3Bitset);
    static int calcActiveBitCount(const uint32_t bits);
};

}//namespace base
}//namespace ems
