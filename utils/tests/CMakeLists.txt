add_executable(SyncSocketTest SyncSocketTest.cpp)
target_link_libraries(SyncSocketTest
PRIVATE cat_utils
)
add_executable(TcpServerTest TcpServerTest.cpp)
target_link_libraries(TcpServerTest
PRIVATE cat_utils
)

# ####################################################
# add_executable(TcpServerTest2 TcpServerTest2.cpp)
# target_link_libraries(TcpServerTest2
# PRIVATE cat_utils
# )

####################################################
add_executable(SyncSocketRespondTest SyncSocketRespondTest.cpp)
target_link_libraries(SyncSocketRespondTest
PRIVATE cat_utils
)

####################################################
add_executable(dealerTest DealerTest.cpp)
target_link_libraries(dealerTest
PRIVATE cat_utils
)

####################################################
add_executable(dealerTest2 DealerTest2.cpp)
target_link_libraries(dealerTest2
PRIVATE cat_utils
)

####################################################
add_executable(boostTimerTest BoostTimerTest.cpp)
target_link_libraries(boostTimerTest
PRIVATE cat_utils
)

####################################################
add_executable(syncRespondTest SyncRespondTest.cpp)
target_link_libraries(syncRespondTest
PRIVATE cat_utils
)