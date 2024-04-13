# 关于第三方依赖：
# 第1类：仅头文件库、以静态库方式引入的库、测试库，都属于开发依赖
# 第2类：以动态库的方式参与开发和生产环境的运行；

set(3rd_libs_PATH $ENV{HOME}/devtool)

# yaml-cpp-0.8.0
set(YAMLCPP_PATH ${3rd_libs_PATH}/yaml-cpp-0.8.0)
include_directories(${YAMLCPP_PATH}/include)
link_directories(${YAMLCPP_PATH}/lib)

# log4cpp-1.1.3
set(LOG4CPP_PATH ${3rd_libs_PATH}/log4cpp-1.1.4)
include_directories(${LOG4CPP_PATH}/include)
link_directories(${LOG4CPP_PATH}/lib)

# boost-1.84.0
set(BOOST_PATH ${3rd_libs_PATH}/boost-1.84.0)
include_directories(${BOOST_PATH}/include)
link_directories(${BOOST_PATH}/lib)

# hiredis-1.2.0
set(HIREDIS_PATH ${3rd_libs_PATH}/hiredis-1.2.0)
include_directories(${HIREDIS_PATH}/include)
link_directories(${HIREDIS_PATH}/lib)

# libzmq-4.3.5
set(LIBZMQ_PATH ${3rd_libs_PATH}/libzmq-4.3.5)
include_directories(${LIBZMQ_PATH}/include)
link_directories(${LIBZMQ_PATH}/lib)

# cppzmq-4.10.0
set(CPPZMQ_PATH ${3rd_libs_PATH}/cppzmq-4.10.0)
include_directories(${CPPZMQ_PATH}/include)

# 注意，此版本在通过cmake间接链接hiredis时，无法自动关联它的运行时路径，暂时还找不到解决办法
# 一种临时手段时，使用它的静态库。即执行target_link_library时，使用redis++.a
# redis-plus-plus-1.3.11
set(REDIS_PLUS_PLUS_PATH ${3rd_libs_PATH}/redis-plus-plus-1.3.12)
include_directories(${REDIS_PLUS_PLUS_PATH}/include)
link_directories(${REDIS_PLUS_PLUS_PATH}/lib)
