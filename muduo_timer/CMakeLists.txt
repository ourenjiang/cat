add_library(muduo_timer
Timestamp.cc
Channel.cc
CurrentThread.cc
EventLoop.cc
EPollPoller.cc
PollPoller.cc
Poller.cc
DefaultPoller.cc
SocketsOps.cc
Timer.cc
TimerQueue.cc
Timestamp.cc
Thread.cc
CountDownLatch.cc
Exception.cc
)

add_executable(main
main.cpp
)

target_link_libraries(main
muduo_timer
)
