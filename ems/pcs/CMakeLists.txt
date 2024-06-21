set(pcs_SRCS
Poller.cpp
PollForward.cpp
Setting.cpp
Controler.cpp
Simulator.cpp
RealtimeWarning.cpp
)

add_library(pcs ${pcs_SRCS})

set_target_properties(pcs PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(pcs
cat_utils
)

add_subdirectory(tests)
