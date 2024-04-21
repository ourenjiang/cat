set(CMAKE_BUILD_TYPE Debug)

# CPP标准
# redis-plus-plus默认使用17版本构建（可以使用低版本重构），要求整个工程也必须与此我版本保持一致；
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_SOURCE_DIR}/myself)
link_directories(${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS
-Wall
)

# set(CMAKE_CXX_CLANG_TIDY
#     "clang-tidy;--enable-check-profile;--checks=-*,
#     abseil-string-find-startswith,bugprone-*,cert-*,
#     clang-analyzer-*,cppcoreguidelines-*,google-*,hicpp-*,
#     llvm-*,misc-*,modernize-*,-modernize-use-trailing-return-type,
#     performance-*,readability-*,-readability-static-definition-in-anonymous-namespace,
#     -readability-simplify-boolean-expr,portability-*")

# # 可用格式1
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;\
#     --enable-check-profile;\
#     --checks=-*,google-*")

# 可用格式2
# 启用检查器配置:引用工程中.clang-tidy配置中的配置项。实测中，我将该配置文件放在源码根目录下。
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy; --enable-check-profile;")
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy; -check=*;")
set(CMAKE_CXX_CLANG_TIDY clang-tidy;
    -config-file=${PROJECT_SOURCE_DIR}/.clang-tidy;)
