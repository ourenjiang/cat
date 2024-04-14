# 关于第三方依赖：
# 第1类：仅头文件库、以静态库方式引入的库、测试库，都属于开发依赖
# 第2类：以动态库的方式参与开发和生产环境的运行；

# benchmark-1.8.3
set(BENCHMARK_PATH $ENV{HOME}/devtool/benchmark-1.8.3)
include_directories(${BENCHMARK_PATH}/include)
link_directories(${BENCHMARK_PATH}/lib)
