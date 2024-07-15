set(base_SRCS
UserManager.cpp
OperationRecord.cpp
BauWarning.cpp
StationInfo.cpp
HistoryWarning.cpp
Interface.cpp
Modbus.cpp
Database.cpp
)

add_library(base ${base_SRCS})

set_target_properties(base PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(base
cat_utils
sqlite3.a
)

# add_subdirectory(tests)
