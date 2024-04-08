message("start building examples")

add_executable(commandline_params_example1
commandline_params_example1.cpp
)

target_link_libraries(commandline_params_example1
cat_utils
)

##################################################
add_executable(environment_variables_example1
environment_variables_example1.cpp
)

target_link_libraries(environment_variables_example1
cat_utils
)

##################################################
add_executable(log4cpp_wrapper_example1
log4cpp_wrapper_example1.cpp
)

target_link_libraries(log4cpp_wrapper_example1
cat_utils
)

##################################################
add_executable(yamlcpp_wrapper_example1
yamlcpp_wrapper_example1.cpp
)

target_link_libraries(yamlcpp_wrapper_example1
cat_utils
)
