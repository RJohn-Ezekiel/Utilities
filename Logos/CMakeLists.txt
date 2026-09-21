cmake_minimum_required(VERSION 3.20)
project(Logos VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# ---------------------------------------------------------------------------
# AreteCore (shared core library)
# ---------------------------------------------------------------------------
if(NOT TARGET AreteCore)
    find_package(AreteCore REQUIRED)
endif()

add_subdirectory(src)

add_executable(logos src/main.cpp)
target_compile_definitions(logos PRIVATE
    BIBLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Bibles"
)
target_link_libraries(logos PRIVATE
    bible_core
    bible_data
    bible_storage
    bible_services
    bible_cli
    bible_ui
    AreteCore
    Qt6::Core
    Qt6::Widgets
)

# Install
install(TARGETS logos RUNTIME DESTINATION bin)
install(DIRECTORY Bibles/ DESTINATION share/logos/Bibles)
install(FILES prayer.json hymns.json DESTINATION share/logos)

# Keep editable prayer/hymn JSONs next to the binary (portable).
add_custom_command(TARGET logos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/prayer.json
        ${CMAKE_CURRENT_SOURCE_DIR}/hymns.json
        $<TARGET_FILE_DIR:logos>
    COMMENT "Copying prayer.json / hymns.json next to binary"
)

# Add tests subdirectory if it exists
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()