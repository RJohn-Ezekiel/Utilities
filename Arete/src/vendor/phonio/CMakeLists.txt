# Vendored Phonio engine (local music player).
# Snapshot of Utilities/Phonio/src, embedded as a static library.

find_package(ZLIB REQUIRED)
find_package(TagLib REQUIRED)

file(GLOB_RECURSE PHONIO_SOURCES
    app/*.cpp
    artwork/*.cpp
    core/*.cpp
    database/*.cpp
    library/*.cpp
    lyrics/*.cpp
    metadata/*.cpp
    player/*.cpp
    playlists/*.cpp
    queue/*.cpp
    settings/*.cpp
    ui/*.cpp
)
file(GLOB_RECURSE PHONIO_HEADERS
    app/*.h
    artwork/*.h
    core/*.h
    database/*.h
    library/*.h
    lyrics/*.h
    metadata/*.h
    player/*.h
    playlists/*.h
    queue/*.h
    settings/*.h
    ui/*.h
)

add_library(arete_vendor_phonio STATIC ${PHONIO_SOURCES} ${PHONIO_HEADERS})
target_include_directories(arete_vendor_phonio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(arete_vendor_phonio PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)
target_link_libraries(arete_vendor_phonio PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Sql
    TagLib::TagLib
    ZLIB::ZLIB
    AreteCore
)
