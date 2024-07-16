cmake_minimum_required(VERSION 3.7)
project(paceic_ems_server)

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

# 编译器参数
add_compile_options(-Wfatal-errors)    # 减少模板代码错误导致的错误信息冗杂的问题
add_compile_options(-Wunused)

#安装路径
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
#添加运行库路径(注意：多个路径之间以分号隔开)
# 说明：${CMAKE_INSTALL_PREFIX}/lib : 用于开发环境
#       /opt/paceic_ems_server/lib : 用于生产环境
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;")

# 安装项目配置文件
install(DIRECTORY etc DESTINATION ${CMAKE_INSTALL_PREFIX} USE_SOURCE_PERMISSIONS)
install(PROGRAMS install.sh DESTINATION ${CMAKE_INSTALL_PREFIX}
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE OWNER_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ)
# install(FILES install.sh DESTINATION ${CMAKE_INSTALL_PREFIX})

#全局头文件路径
include_directories(${PROJECT_SOURCE_DIR})
#全局库文件路径
link_directories(${CMAKE_BINARY_DIR}/lib)

# 关于第三方依赖：
# 第1类：仅头文件库、以静态库方式引入的库、测试库，都属于开发依赖
# 第2类：以动态库的方式参与开发和生产环境的运行；

# 第三方依赖路径：与项目路径同级
set(THIRD_PARTY_PATH ../third_party)

# jsoncpp-1.9.5
set(JSONCPP_PATH ${THIRD_PARTY_PATH}/jsoncpp-1.9.5)
include_directories(${JSONCPP_PATH}/include)
link_directories(${JSONCPP_PATH}/lib)

# libmodbus-3.1.10
set(LIB_MODBUS_PATH ${THIRD_PARTY_PATH}/libmodbus-3.1.10)
include_directories(${LIB_MODBUS_PATH}/include)
link_directories(${LIB_MODBUS_PATH}/lib)

# hiredis-1.2.0
set(HIREDIS_PATH ${THIRD_PARTY_PATH}/hiredis-1.2.0)
include_directories(${HIREDIS_PATH}/include)
link_directories(${HIREDIS_PATH}/lib)

# 注意，此版本在通过cmake间接链接hiredis时，无法自动关联它的运行时路径，暂时还找不到解决办法
# 一种临时手段时，使用它的静态库。即执行target_link_library时，使用redis++.a
# redis-plus-plus-1.3.11
set(REDIS_PLUS_PLUS_PATH ${THIRD_PARTY_PATH}/redis-plus-plus-1.3.11)
include_directories(${REDIS_PLUS_PLUS_PATH}/include)
link_directories(${REDIS_PLUS_PLUS_PATH}/lib)

# libzmq-4.3.5
set(LIBZMQ_PATH ${THIRD_PARTY_PATH}/libzmq-4.3.5)
include_directories(${LIBZMQ_PATH}/include)
link_directories(${LIBZMQ_PATH}/lib)

# cppzmq-4.10.0
set(CPPZMQ_PATH ${THIRD_PARTY_PATH}/cppzmq-4.10.0)
include_directories(${CPPZMQ_PATH}/include)

# msgpack-c-cpp-6.1.0
set(MSGPACK_PATH ${THIRD_PARTY_PATH}/msgpack-c-cpp-6.1.0)
include_directories(${MSGPACK_PATH}/include)

# sqlite-3.39.4
set(SQLITE_PATH ${THIRD_PARTY_PATH}/sqlite-3.39.4)
include_directories(${SQLITE_PATH}/include)
link_directories(${SQLITE_PATH}/lib)

# sqlite_modern_cpp-3.2
set(SQLITE_MODERN_CPP_PATH ${THIRD_PARTY_PATH}/sqlite_modern_cpp-3.2)
include_directories(${SQLITE_MODERN_CPP_PATH}/include)

# boost-1.84.0
set(BOOST_PATH ${THIRD_PARTY_PATH}/boost-1.84.0)
include_directories(${BOOST_PATH}/include)
link_directories(${BOOST_PATH}/lib)

# log4cpp-1.1.3
set(LOG4CPP_PATH ${THIRD_PARTY_PATH}/log4cpp-1.1.3)
include_directories(${LOG4CPP_PATH}/include)
link_directories(${LOG4CPP_PATH}/lib)

# yaml-cpp-0.8.0
set(YAMLCPP_PATH ${THIRD_PARTY_PATH}/yaml-cpp-0.8.0)
include_directories(${YAMLCPP_PATH}/include)
link_directories(${YAMLCPP_PATH}/lib)

add_subdirectory(utils)
add_subdirectory(ems)

install(TARGETS station_main
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
