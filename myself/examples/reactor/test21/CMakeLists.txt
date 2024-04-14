
add_executable(test21
test21.cpp
)

target_link_libraries(test21
myself_communicate_tcp
pthread
)