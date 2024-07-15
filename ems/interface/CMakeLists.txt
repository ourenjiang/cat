set(interface_SRCS
Interface.cpp
HttpServer.cpp
UserManager.cpp
MainWiringDiagram.cpp
StorageEnergySystem.cpp
PublicInfo.cpp
HistoryWarning.cpp
RealtimeWarning.cpp
OperationRecord.cpp
DeviceTreelist.cpp
)

add_library(interface ${interface_SRCS})

set_target_properties(interface PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(interface
profit
xftg
solar
base
)

# add_subdirectory(tests)
