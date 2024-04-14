# 关于第三方依赖：
# 第1类：仅头文件库、以静态库方式引入的库、测试库，都属于开发依赖
# 第2类：以动态库的方式参与开发和生产环境的运行；

# boost-1.81.0
set(BOOST_PATH $ENV{HOME}/devtool/boost-1.81.0)
include_directories(${BOOST_PATH}/include)
link_directories(${BOOST_PATH}/lib)
