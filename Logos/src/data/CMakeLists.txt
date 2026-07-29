find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(bible_data
    KJVImporter.cpp
    VulgateImporter.cpp
    BookNameMatcher.cpp
    Loader.cpp
)

target_include_directories(bible_data PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bible_data PUBLIC bible_core Qt6::Core)
