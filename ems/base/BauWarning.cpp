#include "BauWarning.h"

using namespace ems;
using namespace ems::base;

int BauWarning::calcUniformLevel(const int bitIndex,
        const bitset<32> level1Bitset, const bitset<32> level2Bitset, const bitset<32> level3Bitset)
{
    if(level3Bitset[bitIndex]){
        return 3;
    }
    if(level2Bitset[bitIndex]){
        return 2;
    }
    if(level1Bitset[bitIndex]){
        return 1;
    }
    return 0;
}

int BauWarning::calcActiveBitCount(const uint32_t bits)
{
    const bitset<32> bitParsed{ bits };
    return static_cast<int>(bitParsed.count());
}
