add_library(chronos_cli STATIC
    CommandLineParser.cpp
)

target_include_directories(chronos_cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(chronos_cli PUBLIC Qt6::Core)
