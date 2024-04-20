set(CMAKE_BUILD_TYPE Debug)

# CPP标准
# redis-plus-plus默认使用17版本构建（可以使用低版本重构），要求整个工程也必须与此我版本保持一致；
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_SOURCE_DIR}/htop-3.0.0beta5)
include_directories(${CMAKE_SOURCE_DIR}/htop-3.0.0beta5/linux)
include_directories(${CMAKE_SOURCE_DIR}/htop-3.0.0beta5/base)
include_directories(${CMAKE_SOURCE_DIR}/htop-3.0.0beta5/meter)
include_directories(${CMAKE_SOURCE_DIR}/htop-3.0.0beta5/panel)
include_directories(${CMAKE_SOURCE_DIR}/htop-3.0.0beta5/screen)
link_directories(${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS
-Wall
)