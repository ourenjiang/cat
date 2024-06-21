set(xftg_SRCS
StrategyXftg.cpp
DayPlanDuration.cpp
DayPlanProtect.cpp
WeekPlan.cpp
Setting.cpp
)

add_library(xftg ${xftg_SRCS})
target_link_libraries(xftg
bau
pcs
)
