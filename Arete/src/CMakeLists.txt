add_subdirectory(vendor/chronos)
add_subdirectory(vendor/logos)
add_subdirectory(vendor/codex)
add_subdirectory(vendor/phonio)

file(GLOB_RECURSE ARETE_SOURCES CONFIGURE_DEPENDS
    app/*.cpp
    core/*.cpp
    data/*.cpp
    models/*.cpp
    services/*.cpp
    ui/*.cpp
)
file(GLOB_RECURSE ARETE_HEADERS CONFIGURE_DEPENDS
    app/*.h
    core/*.h
    data/*.h
    models/*.h
    services/*.h
    ui/*.h
)

add_executable(arete ${ARETE_SOURCES} ${ARETE_HEADERS} main.cpp)

target_include_directories(arete PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

target_compile_definitions(arete PRIVATE
    ARETE_LOGOS_BIBLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../Logos/Bibles"
    ARETE_APP_VERSION="${PROJECT_VERSION}"
    ARETE_GITHUB_REPO="RJohn-Ezekiel/Utilities"
    ARETE_GITHUB_ASSET="arete.bin"
)

target_compile_options(arete PRIVATE -Wall -Wextra)

target_link_libraries(arete PRIVATE
    arete_vendor_chronos
    arete_vendor_logos
    arete_vendor_codex
    arete_vendor_phonio
    AreteCore
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Multimedia
    Qt6::Concurrent
)

install(TARGETS arete RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
