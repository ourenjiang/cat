add_library(myself_communicate_tcp SHARED
InetAddress.cpp
Socket.cpp

Acceptor.cpp
TcpServer.cpp
TcpConnection.cpp
Connector.cpp
TcpClient.cpp
)

target_link_libraries(myself_communicate_tcp
myself_reactor
)