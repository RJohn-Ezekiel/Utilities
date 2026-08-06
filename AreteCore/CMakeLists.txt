cmake_minimum_required(VERSION 3.22)
project(AreteCore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Sql DBus)

# Main library
file(GLOB_RECURSE ARETE_SOURCES src/*.cpp)
file(GLOB_RECURSE ARETE_HEADERS include/arete/*.h include/arete/*/*.h)

add_library(AreteCore SHARED ${ARETE_SOURCES} ${ARETE_HEADERS})
target_include_directories(AreteCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/arete>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(AreteCore PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::DBus
)

target_compile_options(AreteCore PRIVATE -Wall -Wextra)

# Install
include(GNUInstallDirs)
install(TARGETS AreteCore
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/arete/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/arete)

# Tests
option(ARETE_BUILD_TESTS "Build tests" OFF)
if(ARETE_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()