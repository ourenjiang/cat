
add_executable(protobuf_test01
test01.cpp
Person.pb.cc
)

target_link_libraries(protobuf_test01
protobuf
)