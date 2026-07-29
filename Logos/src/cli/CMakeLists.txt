add_library(bible_cli
    CLIHandler.cpp
)

target_include_directories(bible_cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bible_cli PUBLIC bible_core bible_data bible_services)
