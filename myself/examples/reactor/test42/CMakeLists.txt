
add_executable(test42
test42.cpp
)

target_link_libraries(test42
myself_communicate_domain
pthread
)