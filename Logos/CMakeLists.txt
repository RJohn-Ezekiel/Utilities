cmake_minimum_required(VERSION 3.20)
project(BibleExplorer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_subdirectory(src)

add_executable(bible src/main.cpp)
target_compile_definitions(bible PRIVATE
    BIBLES_DIR="${CMAKE_SOURCE_DIR}/Bibles"
)
target_link_libraries(bible PRIVATE
    bible_core
    bible_data
    bible_storage
    bible_services
    bible_cli
    bible_ui
    Qt6::Core
    Qt6::Widgets
)

# Install
install(TARGETS bible RUNTIME DESTINATION bin)
install(DIRECTORY Bibles/ DESTINATION share/bible-explorer/Bibles)

# Add tests subdirectory if it exists
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
