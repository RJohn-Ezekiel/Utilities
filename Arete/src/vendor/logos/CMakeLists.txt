# Vendored Logos engine (Bible explorer, hymns, prayers).
# Snapshot of Utilities/Logos/src, embedded as a static library.

file(GLOB_RECURSE LOGOS_SOURCES
    core/*.cpp
    data/*.cpp
    services/*.cpp
    storage/*.cpp
    ui/*.cpp
)
file(GLOB_RECURSE LOGOS_HEADERS
    core/*.h
    data/*.h
    services/*.h
    storage/*.h
    ui/*.h
)

add_library(arete_vendor_logos STATIC ${LOGOS_SOURCES} ${LOGOS_HEADERS})
target_include_directories(arete_vendor_logos PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(arete_vendor_logos PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    AreteCore
)
