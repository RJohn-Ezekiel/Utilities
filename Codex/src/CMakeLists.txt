file(GLOB_RECURSE CODEX_SOURCES
    core/*.cpp core/*.h
    markdown/*.cpp markdown/*.h
    index/*.cpp index/*.h
    media/*.cpp media/*.h
    export/*.cpp export/*.h
    cli/*.cpp cli/*.h
    ui/*.cpp ui/*.h
)

add_library(codex_lib STATIC ${CODEX_SOURCES})
target_include_directories(codex_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(codex_lib PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets)

add_executable(codex main.cpp)
target_link_libraries(codex PRIVATE codex_lib)
