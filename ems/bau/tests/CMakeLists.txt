set(bau_interface_test_SRCS
bau_interface_test.cpp
)

add_executable(bau_interface_test ${bau_interface_test_SRCS})
target_link_libraries(bau_interface_test
bau
)

add_executable(bau_simulator_test simulator_test.cpp)
target_link_libraries(bau_simulator_test
bau
)

add_executable(bau_router_test BauRouterTest.cpp)
target_link_libraries(bau_router_test
bau
)

add_executable(bau_poller_test BauPollerTest.cpp)
target_link_libraries(bau_poller_test
bau
)

add_executable(bau_collector_builder_test BauCollectorBuilderTest.cpp)
target_link_libraries(bau_collector_builder_test
bau
)
