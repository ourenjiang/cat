cmake_minimum_required(VERSION 3.15)
project(cat)

#编译类型
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)

# CPP标准
# redis-plus-plus默认使用17版本构建（可以使用低版本重构），要求整个工程也必须与此我版本保持一致；
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 全局参数
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#安装路径
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)

#全局头文件路径
include_directories(${PROJECT_SOURCE_DIR})

# 关于第三方依赖：
# 第1类：仅头文件库、以静态库方式引入的库、测试库，都属于开发依赖
# 第2类：以动态库的方式参与开发和生产环境的运行；

# yaml-cpp-0.8.0
set(YAMLCPP_PATH $ENV{HOME}/devtools/yaml-cpp-0.8.0)
include_directories(${YAMLCPP_PATH}/include)
link_directories(${YAMLCPP_PATH}/lib)

# log4cpp-1.1.3
set(LOG4CPP_PATH $ENV{HOME}/devtools/log4cpp-1.1.3)
include_directories(${LOG4CPP_PATH}/include)
link_directories(${LOG4CPP_PATH}/lib)

# boost-1.84.0
set(BOOST_PATH $ENV{HOME}/devtools/boost-1.84.0)
include_directories(${BOOST_PATH}/include)
link_directories(${BOOST_PATH}/lib)

add_subdirectory(cat)
add_subdirectory(tests)
add_subdirectory(examples)
