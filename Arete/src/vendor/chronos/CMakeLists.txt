# Vendored Chronos engine (timer, sessions, statistics, UI).
# Snapshot of Utilities/Chronos/src, embedded as a static library.

file(GLOB_RECURSE CHRONOS_SOURCES
    core/*.cpp
    models/*.cpp
    storage/*.cpp
    services/*.cpp
    ui/*.cpp
)
file(GLOB_RECURSE CHRONOS_HEADERS
    core/*.h
    models/*.h
    storage/*.h
    services/*.h
    ui/*.h
)

add_library(arete_vendor_chronos STATIC ${CHRONOS_SOURCES} ${CHRONOS_HEADERS})
target_include_directories(arete_vendor_chronos PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(arete_vendor_chronos PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    AreteCore
)
