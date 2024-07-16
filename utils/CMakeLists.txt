set(utils_SRCS
ResourceInit.cpp
Log4cppWrapper.cpp
YamlcppWrapper.cpp
CommandlineParams.cpp
EnvironmentVariables.cpp
zeromq.cpp
Miscellaneous.cpp
HttpWrapper.cpp
cpphttplib_splitout/httplib.cc
SyncRequest.cpp
SyncRespond.cpp
BoostTimer.cpp
crc16.cpp
datetime.cpp
endian.cpp
cgi.cpp
ModbusRtu.cpp
jsonWrapper.cpp
StreamWrapper.cpp
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
boost_timer.a
)

add_subdirectory(tests)
# add_subdirectory(C++)
