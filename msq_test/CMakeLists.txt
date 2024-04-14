add_executable(SystemV_send_test
SystemV_send_test.cpp
)

add_executable(SystemV_recv_test
SystemV_recv_test.cpp
)

add_executable(Posix_send_test
Posix_send_test.cpp
)

add_executable(Posix_recv_test
Posix_recv_test.cpp
)

target_link_libraries(Posix_send_test
rt
)

target_link_libraries(Posix_recv_test
rt
)
