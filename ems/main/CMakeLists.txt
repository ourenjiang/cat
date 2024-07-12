add_executable(station_main
main.cpp
)

target_link_libraries(station_main
station
interface
processor
)

# add_subdirectory(tests)
