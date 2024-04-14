#define CATCH_CONFIG_MAIN
#include "catch2/catch_all.hpp"

int sum1(int num)
{
	int sum = 0;
	for(; num; --num)
	{
		sum += num;
	}
	return sum;
}

int sum2(int num)
{
	return num * ( 1 + num ) / 2;
}

TEST_CASE()
{
	REQUIRE(sum1(2) == sum2(2));
}
