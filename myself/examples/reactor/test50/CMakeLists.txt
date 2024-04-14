
add_executable(test50
test50.cpp
)

target_link_libraries(test50
myself_communicate_serial
pthread
)
