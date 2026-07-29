cmake_minimum_required(VERSION 3.22)
project(Codex
    VERSION 1.0.0
    DESCRIPTION "Offline-first Markdown knowledge base"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_subdirectory(src)

option(CODEX_BUILD_TESTS "Build tests" OFF)
if(CODEX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
