set(bau_SRCS
Setting.cpp
CellSystem.cpp
HeapSystem.cpp
DeviceStatus.cpp
Simulator.cpp
RealtimeWarning.cpp
BauCollector.cpp
)

add_library(bau ${bau_SRCS})

set_target_properties(bau PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(bau
base
# cat_utils
)

# add_subdirectory(tests)
