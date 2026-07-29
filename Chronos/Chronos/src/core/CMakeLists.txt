add_library(chronos_core STATIC
    TimerEngine.cpp
)

target_include_directories(chronos_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(chronos_core PUBLIC Qt6::Core)
