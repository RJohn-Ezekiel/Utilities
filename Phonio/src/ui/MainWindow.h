#pragma once

#include "core/Types.h"

#include <QMainWindow>
#include <QVector>

class QListWidget;
class QStackedWidget;
class QLabel;

namespace phonio {

class App;
class SettingsManager;
class LibraryManager;
class PlaybackController;
class PlaylistManager;
class QueueManager;
class LibraryScanner;
class ArtworkManager;
class LyricsManager;
class MetadataManager;

class PlaybackBar;
class NowPlayingWidget;
class LibraryPage;
class ArtistsPage;
class AlbumsPage;
class GenresPage;
class PlaylistsPage;
class QueuePage;
class SettingsPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(App* app, QWidget* parent = nullptr);

    void setNowPlayingPage(NowPlayingWidget* page);
    void showNowPlaying();

private slots:
    void onSidebarChanged(int row);
    void onCurrentTrackChanged(const Track& track);
    void onPlaybackStateChanged();

private:
    void showBrowserPage(int stackIndex);
    void handleEditMetadata(const Track& track);
    void handleAttachLyrics(const Track& track);
    void ensureScanStarted();

    App* m_app;
    SettingsManager* m_settings;
    LibraryManager* m_library;
    PlaybackController* m_controller;
    PlaylistManager* m_playlists;
    QueueManager* m_queue;
    LibraryScanner* m_scanner;
    ArtworkManager* m_artwork;
    LyricsManager* m_lyrics;
    MetadataManager* m_metadata;

    QListWidget* m_sidebar;
    QStackedWidget* m_stack;
    PlaybackBar* m_playbackBar;
    NowPlayingWidget* m_nowPlaying = nullptr;
    int m_nowPlayingIndex = -1;
    int m_browserStackIndex = 0;
    bool m_scanStarted = false;

    LibraryPage* m_libraryPage;
    ArtistsPage* m_artistsPage;
    AlbumsPage* m_albumsPage;
    GenresPage* m_genresPage;
    PlaylistsPage* m_playlistsPage;
    QueuePage* m_queuePage;
    SettingsPage* m_settingsPage;
};

} // namespace phonio
