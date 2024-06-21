set(xftg_interface_test_SRCS
interface_test.cpp
)

add_executable(xftg_interface_test ${xftg_interface_test_SRCS})
target_link_libraries(xftg_interface_test
xftg_strategy
)

#####################################################################
set(xftg_execute_test_SRCS
execute_test.cpp
)

add_executable(xftg_execute_test ${xftg_execute_test_SRCS})
target_link_libraries(xftg_execute_test
xftg_strategy
)

#####################################################################
set(xftg_protect_test_SRCS
protect_test.cpp
)

add_executable(xftg_protect_test ${xftg_protect_test_SRCS})
target_link_libraries(xftg_protect_test
xftg_strategy
)
