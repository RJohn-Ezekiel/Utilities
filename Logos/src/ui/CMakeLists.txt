find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_library(bible_ui
    Theme.cpp
    MainWindow.cpp
    Sidebar.cpp
    BookTree.cpp
    ReaderPane.cpp
    SideBySideReader.cpp
    SearchBar.cpp
    NotesPanel.cpp
    VersePanel.cpp
    SettingsDialog.cpp
)

target_include_directories(bible_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bible_ui PUBLIC
    bible_core
    bible_data
    bible_services
    bible_storage
    Qt6::Core
    Qt6::Widgets
)
