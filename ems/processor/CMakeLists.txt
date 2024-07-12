set(processor_SRCS
Processor.cpp
BauHistoryWarning.cpp
)

add_library(processor ${processor_SRCS})

set_target_properties(processor PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(processor
cat_utils
)

# add_subdirectory(tests)
