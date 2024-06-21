set(solar_SRCS
Setting.cpp
)

add_library(solar ${solar_SRCS})

set_target_properties(solar PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(solar
cat_utils
)
