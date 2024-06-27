set(utils_SRCS
ResourceInit.cpp
Log4cppWrapper.cpp
YamlcppWrapper.cpp
CommandlineParams.cpp
EnvironmentVariables.cpp
ZmqRequest.cpp
ZmqRespond.cpp
ZmqPublish.cpp
ZmqSubscribe.cpp
ZmqSocket.cpp
Miscellaneous.cpp
HttpWrapper.cpp
cpphttplib_splitout/httplib.cc
SyncSocketRequest.cpp
SyncSocketRespond.cpp
)

add_library(cat_utils
${utils_SRCS}
)

set_target_properties(cat_utils PROPERTIES POSITION_INDEPENDENT_CODE ON) # -fPIC for static library
target_link_libraries(cat_utils
yaml-cpp.a
log4cpp.a
boost_program_options.a
zmq.a
modbus.a
jsoncpp.a
)

add_subdirectory(tests)
add_subdirectory(C++)
