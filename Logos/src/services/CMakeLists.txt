add_library(bible_services
    SearchService.cpp
    VerseService.cpp
    ReferenceParser.cpp
)

target_include_directories(bible_services PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bible_services PUBLIC bible_core bible_data)
