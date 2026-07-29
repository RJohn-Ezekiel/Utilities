cmake_minimum_required(VERSION 3.20)
project(visio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)

# ============================================================================
# Dependency checks — warn if runtime tools are missing
# ============================================================================

macro(check_runtime_dependency NAME EXECUTABLE)
    find_program(VISIO_PROG_${NAME} ${EXECUTABLE})
    if(NOT VISIO_PROG_${NAME})
        message("")
        message(STATUS "${NAME} (${EXECUTABLE}) not found.")
        message(STATUS "Install it:")
        message(STATUS "  Debian/Ubuntu: sudo apt install ${EXECUTABLE}")
        message(STATUS "  Fedora:        sudo dnf install ${EXECUTABLE}")
        message(STATUS "  Arch:          sudo pacman -S ${EXECUTABLE}")
        message(STATUS "  macOS:         brew install ${EXECUTABLE}")
        message("")
    else()
        message(STATUS "Found ${NAME}: ${VISIO_PROG_${NAME}}")
    endif()
endmacro()

check_runtime_dependency(curl   curl)
check_runtime_dependency(ytdlp  yt-dlp)
check_runtime_dependency(mpv    mpv)

# ============================================================================
# Dependencies — fetched via CMake
# ============================================================================

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(nlohmann_json)

# Qt 6 (required for GUI)
find_package(Qt6 COMPONENTS Widgets Network REQUIRED)
message(STATUS "Found Qt6: ${Qt6_VERSION}")
set(VISIO_QT_FOUND TRUE)

# ============================================================================
# Library
# ============================================================================

file(GLOB_RECURSE VISIO_SOURCES src/*.cpp)
list(REMOVE_ITEM VISIO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http_util.hpp
)
file(GLOB_RECURSE VISIO_GUI_SOURCES src/gui/*.cpp)
list(REMOVE_ITEM VISIO_SOURCES ${VISIO_GUI_SOURCES})

add_library(visio ${VISIO_SOURCES})

target_include_directories(visio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(visio PUBLIC nlohmann_json::nlohmann_json Qt6::Core)
target_compile_options(visio PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(visio PRIVATE VISIO_HAVE_QT=1)

# ============================================================================
# Tests
# ============================================================================

option(VISIO_BUILD_TESTS "Build tests" ON)
if(VISIO_BUILD_TESTS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.7.0
        GIT_SHALLOW ON
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(visio_tests
        tests/test_client.cpp
        tests/test_search.cpp
        tests/test_types.cpp
    )
    target_link_libraries(visio_tests PRIVATE visio Catch2::Catch2WithMain)
endif()

# ============================================================================
# Examples
# ============================================================================

option(VISIO_BUILD_EXAMPLES "Build examples" ON)
if(VISIO_BUILD_EXAMPLES)
    add_executable(visio_search_example examples/search_example.cpp)
    target_link_libraries(visio_search_example PRIVATE visio)

    add_executable(visio_video_info_example examples/video_info_example.cpp)
    target_link_libraries(visio_video_info_example PRIVATE visio)

    add_executable(visio_queue_example examples/queue_playlist_example.cpp)
    target_link_libraries(visio_queue_example PRIVATE visio)
endif()

# ============================================================================
# GUI Application
# ============================================================================

file(GLOB_RECURSE VISIO_GUI_HEADERS src/gui/*.hpp src/gui/*.h)
add_executable(visio_gui ${VISIO_GUI_SOURCES} ${VISIO_GUI_HEADERS})
target_link_libraries(visio_gui PRIVATE visio Qt6::Widgets Qt6::Network)
