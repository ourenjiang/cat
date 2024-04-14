# 关于第三方依赖：
# 第1类：仅头文件库、以静态库方式引入的库、测试库，都属于开发依赖
# 第2类：以动态库的方式参与开发和生产环境的运行；

set(3rd_libs_PATH $ENV{HOME}/devtool)

# boost-1.81.0
set(BOOST_PATH $ENV{HOME}/devtool/boost-1.81.0)
include_directories(${BOOST_PATH}/include)
link_directories(${BOOST_PATH}/lib)

# sqlite-3.39.4
set(SQLITE_PATH $ENV{HOME}/devtool/sqlite-3.39.4)
include_directories(${SQLITE_PATH}/include)
link_directories(${SQLITE_PATH}/lib)

# sqlite_modern_cpp-3.2
set(SQLITE_MODERN_CPP_PATH $ENV{HOME}/devtool/sqlite_modern_cpp-3.2)
include_directories(${SQLITE_MODERN_CPP_PATH}/include)
# link_directories(${SQLITE_MODERN_CPP_PATH}/lib)

# catch2-3.5.0
set(CATCH2_PATH $ENV{HOME}/devtool/catch2-3.5.0)
include_directories(${CATCH2_PATH}/include)
link_directories(${CATCH2_PATH}/lib)

# protobuf-3.20.3
set(PROTOBUF_PATH $ENV{HOME}/devtool/protobuf-3.20.3)
include_directories(${PROTOBUF_PATH}/include)
link_directories(${PROTOBUF_PATH}/lib)

# libmodbus-3.1.10
set(LIBMODBUS_PATH $ENV{HOME}/devtool/libmodbus-3.1.10)
include_directories(${LIBMODBUS_PATH}/include)
link_directories(${LIBMODBUS_PATH}/lib)

