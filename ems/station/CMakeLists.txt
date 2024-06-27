add_executable(station
Poller.cpp
HttpServer.cpp
UserManager.cpp
MainWiringDiagram.cpp
StorageEnergySystem.cpp
PublicInfo.cpp
HistoryWarning.cpp
RealtimeWarning.cpp
OperationRecord.cpp
DeviceTreelist.cpp
Station.cpp
main.cpp
)

target_link_libraries(station
profit
xftg
solar
boost_timer.a
)

# add_subdirectory(tests)
