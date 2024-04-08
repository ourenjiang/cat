set(utils_SRCS
Log4cppWrapper.cpp
YamlcppWrapper.cpp
CommandlineParams.cpp
EnvironmentVariables.cpp
)

add_library(cat_utils
${utils_SRCS}
)

set_target_properties(cat_utils PROPERTIES POSITION_INDEPENDENT_CODE ON) # -fPIC for static library
target_link_libraries(cat_utils
yaml-cpp.a
log4cpp.a
boost_program_options.a
)
