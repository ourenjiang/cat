set(station_SRCS
Station.cpp
# StationBuilder.cpp
)

add_library(station ${station_SRCS})

set_target_properties(station PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(station
bau
pcs
xftg
)

# add_subdirectory(tests)
