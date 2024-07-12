add_executable(simulator_test
main.cpp
)

target_link_libraries(simulator_test
station
interface
processor
)
