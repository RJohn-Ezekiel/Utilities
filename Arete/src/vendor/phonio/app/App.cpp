#include "app/App.h"

#include "settings/SettingsManager.h"
#include "database/DatabaseManager.h"
#include "metadata/MetadataManager.h"
#include "artwork/ArtworkManager.h"
#include "library/LibraryManager.h"
#include "library/LibraryScanner.h"
#include "lyrics/LyricsManager.h"
#include "player/AudioPlayer.h"
#include "player/PlaybackController.h"
#include "queue/QueueManager.h"
#include "playlists/PlaylistManager.h"
#include "ui/MainWindow.h"
#include "ui/NowPlayingWidget.h"

#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QSet>

namespace phonio {

App::App(QApplication& app, QObject* parent)
    : QObject(parent)
    , m_app(app)
    , m_settings(new SettingsManager(this))
    , m_database(new DatabaseManager(
          QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
              + QStringLiteral("/phonio.db"),
          this))
    , m_metadata(new MetadataManager(this))
    , m_artwork(new ArtworkManager(m_metadata, this))
    , m_library(new LibraryManager(m_database, this))
    , m_scanner(new LibraryScanner(this))
    , m_lyrics(new LyricsManager(m_library, this))
    , m_player(new AudioPlayer(this))
    , m_queue(new QueueManager(m_database, this))
    , m_controller(new PlaybackController(m_player, m_queue, m_library, m_settings, m_lyrics, this))
    , m_playlists(new PlaylistManager(m_database, m_library, this))
    , m_mainWindow(new MainWindow(this))
    , m_nowPlaying(new NowPlayingWidget(m_controller, m_artwork, m_lyrics, m_mainWindow))
{
    m_player->setVolume(m_settings->volume());
    m_queue->loadFromDatabase();
    m_mainWindow->setNowPlayingPage(m_nowPlaying);

    // Wire the scanner into the library.
    connect(m_scanner, &LibraryScanner::scannedTracks, this, [this](const QVector<Track>& tracks) {
        for (const auto& track : tracks)
            m_database->upsertTrack(track);
    });
    connect(m_scanner, &LibraryScanner::finished, this, [this] {
        // Drop records whose files disappeared since the last scan.
        QSet<QString> existingPaths;
        QSet<qint64> existingIds;
        const auto all = m_database->loadAllTracks();
        for (const auto& track : all) {
            if (QFileInfo::exists(track.filePath)) {
                existingPaths.insert(track.filePath);
                existingIds.insert(track.id);
            }
        }
        m_database->removeTracksNotIn(existingPaths);
        m_library->reloadFromDatabase();
        m_queue->trimToExisting(existingIds);
        m_settings->sync();
    });

    // Persist volume changes.
    connect(m_player, &AudioPlayer::volumeChanged, this, [this](double volume) {
        m_settings->setVolume(volume);
    });
}

App::~App()
{
    m_settings->sync();
}

int App::run()
{
    m_mainWindow->show();
    return m_app.exec();
}

} // namespace phonio
