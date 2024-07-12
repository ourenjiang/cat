add_executable(station_builder_test StationBuilderTest.cpp)
target_link_libraries(station_builder_test
station
)

add_executable(station2_test Station2Test.cpp)
target_link_libraries(station2_test
station
)
