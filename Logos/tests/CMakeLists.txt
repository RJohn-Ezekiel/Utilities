# Test executable
find_package(Qt6 REQUIRED COMPONENTS Core Test)

add_executable(test_bible_explorer
    test_reference.cpp
    test_book_name_matcher.cpp
    test_reference_parser.cpp
    test_search.cpp
)

target_include_directories(test_bible_explorer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
)

target_link_libraries(test_bible_explorer PRIVATE
    bible_core
    bible_data
    bible_services
    Qt6::Core
    Qt6::Test
)

add_test(NAME BibleExplorerTests
    COMMAND test_bible_explorer
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
