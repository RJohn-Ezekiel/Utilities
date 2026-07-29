find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(test_timer_engine
    test_timer_engine.cpp
)

target_link_libraries(test_timer_engine PRIVATE
    chronos_core
    chronos_models
    Qt6::Test
    Qt6::Core
)

target_include_directories(test_timer_engine PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME TimerEngine COMMAND test_timer_engine)
