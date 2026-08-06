cmake_minimum_required(VERSION 3.22)
project(Arete VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Sql Multimedia Concurrent Test)

# AreteCore shared library (logging, settings, theme)
if(NOT TARGET AreteCore)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../AreteCore/CMakeLists.txt)
        add_subdirectory(../AreteCore ${CMAKE_CURRENT_BINARY_DIR}/aretecore)
    else()
        find_package(AreteCore REQUIRED)
    endif()
endif()

# UpdateService locates the shared build tree through ARETE_SOURCE_ROOT;
# make the self-update button work even for standalone install.sh builds.
get_filename_component(ARETE_WORKSPACE "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
target_compile_definitions(AreteCore PUBLIC
    ARETE_SOURCE_ROOT="${ARETE_WORKSPACE}")

add_subdirectory(src)

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
