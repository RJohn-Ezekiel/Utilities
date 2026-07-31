cmake_minimum_required(VERSION 3.21)

project(Phonio
    VERSION 0.1.0
    DESCRIPTION "Modern desktop music player"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Multimedia Sql)
find_package(ZLIB REQUIRED)
find_package(TagLib REQUIRED)

qt_standard_project_setup()

set(PHONIO_SOURCES
    src/main.cpp
    src/app/App.cpp
    src/core/Types.cpp
    src/settings/SettingsManager.cpp
    src/database/DatabaseManager.cpp
    src/metadata/MetadataManager.cpp
    src/artwork/ArtworkManager.cpp
    src/library/LibraryScanner.cpp
    src/library/LibraryManager.cpp
    src/lyrics/LyricsParser.cpp
    src/lyrics/LyricsManager.cpp
    src/lyrics/LyricsWidget.cpp
    src/player/AudioPlayer.cpp
    src/player/PlaybackController.cpp
    src/queue/QueueManager.cpp
    src/playlists/PlaylistManager.cpp
    src/ui/MainWindow.cpp
    src/ui/PlaybackBar.cpp
    src/ui/NowPlayingWidget.cpp
    src/ui/LibraryPage.cpp
    src/ui/ArtistsPage.cpp
    src/ui/PlaylistsPage.cpp
    src/ui/QueuePage.cpp
    src/ui/SettingsPage.cpp
    src/ui/MetadataDialog.cpp
    src/ui/SlidingStack.cpp
    src/ui/SongTableView.cpp
    src/ui/TrackListModel.cpp
    src/ui/ArtworkLabel.cpp
    src/ui/TransportIcons.cpp
    src/ui/Theme.cpp
)

set(PHONIO_HEADERS
    src/app/App.h
    src/core/Types.h
    src/settings/SettingsManager.h
    src/database/DatabaseManager.h
    src/metadata/MetadataManager.h
    src/artwork/ArtworkManager.h
    src/library/LibraryScanner.h
    src/library/LibraryManager.h
    src/lyrics/LyricsParser.h
    src/lyrics/LyricsManager.h
    src/lyrics/LyricsWidget.h
    src/player/AudioPlayer.h
    src/player/PlaybackController.h
    src/queue/QueueManager.h
    src/playlists/PlaylistManager.h
    src/ui/MainWindow.h
    src/ui/PlaybackBar.h
    src/ui/NowPlayingWidget.h
    src/ui/LibraryPage.h
    src/ui/ArtistsPage.h
    src/ui/PlaylistsPage.h
    src/ui/QueuePage.h
    src/ui/SettingsPage.h
    src/ui/MetadataDialog.h
    src/ui/SlidingStack.h
    src/ui/SongTableView.h
    src/ui/TrackListModel.h
    src/ui/ArtworkLabel.h
    src/ui/TransportIcons.h
    src/ui/Theme.h
)

qt_add_executable(phonio ${PHONIO_SOURCES} ${PHONIO_HEADERS})

target_include_directories(phonio PRIVATE src)

target_compile_definitions(phonio PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

target_link_libraries(phonio PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Sql
    TagLib::TagLib
)

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS phonio RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(UNIX AND NOT APPLE)
    install(FILES packaging/phonio.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
include(CTest)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)

    set(TEST_COMMON_SOURCES
        src/core/Types.cpp
        src/settings/SettingsManager.cpp
        src/database/DatabaseManager.cpp
        src/metadata/MetadataManager.cpp
        src/library/LibraryManager.cpp
        src/lyrics/LyricsParser.cpp
        src/queue/QueueManager.cpp
        src/playlists/PlaylistManager.cpp
    )

    set(TEST_COMMON_HEADERS
        src/core/Types.h
        src/settings/SettingsManager.h
        src/database/DatabaseManager.h
        src/metadata/MetadataManager.h
        src/library/LibraryManager.h
        src/lyrics/LyricsParser.h
        src/queue/QueueManager.h
        src/playlists/PlaylistManager.h
    )

    function(phonio_add_test name source)
        qt_add_executable(${name} ${source} ${TEST_COMMON_SOURCES} ${TEST_COMMON_HEADERS})
        target_include_directories(${name} PRIVATE src)
        target_link_libraries(${name} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql Qt6::Test TagLib::TagLib)
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    phonio_add_test(tst_lyricsparser tests/tst_lyricsparser.cpp)
    phonio_add_test(tst_database tests/tst_database.cpp)
    phonio_add_test(tst_queuemanager tests/tst_queuemanager.cpp)
endif()
