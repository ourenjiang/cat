#pragma once
#include <sstream>
#include <vector>
#include <stdint.h>

namespace endian
{
using namespace std;

//大小端转换
void reverse(void *data, int len);
void reverseByteArray(void *arr, const unsigned long len);

}////namespace endian
