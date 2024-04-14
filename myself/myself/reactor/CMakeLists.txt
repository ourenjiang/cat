add_library(myself_reactor SHARED
EventLoop.cpp
Poller.cpp
Channel.cpp
Timer.cpp
TimerQueue.cpp
EventLoopThread.cpp
)

target_link_libraries(myself_reactor
myself_base
)