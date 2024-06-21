add_executable(electricity_price_day_plan_test
day_plan_test.cpp
)
target_link_libraries(electricity_price_day_plan_test
profit_strategy
)

#########################################################
add_executable(electricity_price_month_plan_test
month_plan_test.cpp
)
target_link_libraries(electricity_price_month_plan_test
profit_strategy
)

#########################################################
add_executable(electricity_price_typelist_test
typelist_test.cpp
)
target_link_libraries(electricity_price_typelist_test
profit_strategy
)
