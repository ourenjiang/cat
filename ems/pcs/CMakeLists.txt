set(pcs_SRCS
PcsCollector.cpp
Setting.cpp
Simulator.cpp
RealtimeWarning.cpp
)

add_library(pcs ${pcs_SRCS})

set_target_properties(pcs PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(pcs
base
)

# add_subdirectory(tests)
