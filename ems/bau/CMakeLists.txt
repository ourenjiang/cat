set(bau_SRCS
BauPoller.cpp
BcuPoller.cpp
BmuPoller.cpp
Setting.cpp
CellSystem.cpp
HeapSystem.cpp
Controler.cpp
PollForward.cpp
Simulator.cpp
RealtimeWarning.cpp
)

add_library(bau ${bau_SRCS})

set_target_properties(bau PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(bau
cat_utils
)

add_subdirectory(tests)
