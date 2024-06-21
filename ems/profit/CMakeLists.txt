set(profit_SRCS
DayPlan.cpp
MonthPlan.cpp
TypeList.cpp
ProfitHomepage.cpp
)

add_library(profit ${profit_SRCS})
target_link_libraries(profit
cat_utils
sqlite3.a
)

# add_subdirectory(tests)
