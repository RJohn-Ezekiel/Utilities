#pragma once

#include <QObject>

class QApplication;

namespace phonio {

class SettingsManager;
class DatabaseManager;
class MetadataManager;
class ArtworkManager;
class LibraryManager;
class LibraryScanner;
class LyricsManager;
class AudioPlayer;
class QueueManager;
class PlaybackController;
class PlaylistManager;
class MainWindow;
class NowPlayingWidget;

// Composition root: constructs all services, wires them together with
// constructor injection (no global state) and owns them.
class App : public QObject
{
    Q_OBJECT

public:
    explicit App(QApplication& app, QObject* parent = nullptr);
    ~App() override;

    SettingsManager* settings() const { return m_settings; }
    DatabaseManager* database() const { return m_database; }
    MetadataManager* metadata() const { return m_metadata; }
    ArtworkManager* artwork() const { return m_artwork; }
    LibraryManager* library() const { return m_library; }
    LibraryScanner* scanner() const { return m_scanner; }
    LyricsManager* lyrics() const { return m_lyrics; }
    AudioPlayer* player() const { return m_player; }
    QueueManager* queue() const { return m_queue; }
    PlaybackController* controller() const { return m_controller; }
    PlaylistManager* playlists() const { return m_playlists; }
    MainWindow* mainWindow() const { return m_mainWindow; }

    int run();

private:
    QApplication& m_app;
    SettingsManager* m_settings;
    DatabaseManager* m_database;
    MetadataManager* m_metadata;
    ArtworkManager* m_artwork;
    LibraryManager* m_library;
    LibraryScanner* m_scanner;
    LyricsManager* m_lyrics;
    AudioPlayer* m_player;
    QueueManager* m_queue;
    PlaybackController* m_controller;
    PlaylistManager* m_playlists;
    MainWindow* m_mainWindow;
    NowPlayingWidget* m_nowPlaying;
};

} // namespace phonio
