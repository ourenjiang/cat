set(pcs_setting_interface_test_SRCS
setting_interface_test.cpp
)

add_executable(pcs_setting_interface_test ${pcs_setting_interface_test_SRCS})
target_link_libraries(pcs_setting_interface_test
pcs
)

add_executable(pcs_proxy_test proxy_test.cpp)
target_link_libraries(pcs_proxy_test
cat_utils
)

add_executable(pcs_simulator_test simulator_test.cpp)
target_link_libraries(pcs_simulator_test
pcs
)
