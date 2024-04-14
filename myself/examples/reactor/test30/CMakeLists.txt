
add_executable(test30
test30.cpp
)

target_link_libraries(test30
myself_communicate_tcp
pthread
)