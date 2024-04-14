# 关于第三方依赖：
# 第1类：仅头文件库、以静态库方式引入的库、测试库，都属于开发依赖
# 第2类：以动态库的方式参与开发和生产环境的运行；

set(3rd_libs_PATH $ENV{HOME}/devtool)

# # yaml-cpp-0.8.0
# set(YAMLCPP_PATH ${3rd_libs_PATH}/yaml-cpp-0.8.0)
# include_directories(${YAMLCPP_PATH}/include)
# link_directories(${YAMLCPP_PATH}/lib)

