#include "boost/python.hpp"
#include <iostream>
#include "boost-1.81.0_/include/boost/python.hpp"

int main()
{
	const char* pythonVersion = BOOST_PYTHON_VERSION;
	std::cout << "Version: " << pythonVersion << std::endl;
}
