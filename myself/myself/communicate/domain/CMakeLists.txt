add_library(myself_communicate_domain SHARED
DomainSocket.cpp

DomainAcceptor.cpp
DomainServer.cpp
DomainConnection.cpp
DomainConnector.cpp
DomainClient.cpp
)

target_link_libraries(myself_communicate_domain
myself_reactor
)