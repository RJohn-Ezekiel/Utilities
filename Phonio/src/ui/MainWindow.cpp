#include "ui/MainWindow.h"

#include "app/App.h"
#include "settings/SettingsManager.h"
#include "library/LibraryManager.h"
#include "library/LibraryScanner.h"
#include "player/PlaybackController.h"
#include "playlists/PlaylistManager.h"
#include "queue/QueueManager.h"
#include "artwork/ArtworkManager.h"
#include "lyrics/LyricsManager.h"
#include "metadata/MetadataManager.h"

#include "ui/PlaybackBar.h"
#include "ui/NowPlayingWidget.h"
#include "ui/LibraryPage.h"
#include "ui/ArtistsPage.h"
#include "ui/PlaylistsPage.h"
#include "ui/QueuePage.h"
#include "ui/SettingsPage.h"
#include "ui/MetadataDialog.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QStatusBar>
#include <QStyle>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

namespace phonio {

namespace {

enum SidebarItem {
    SidebarLibrary,
    SidebarArtists,
    SidebarAlbums,
    SidebarPlaylists,
    SidebarGenres,
    SidebarQueue,
    SidebarSettings,
    SidebarCount,
};

} // namespace

MainWindow::MainWindow(App* app, QWidget* parent)
    : QMainWindow(parent)
    , m_app(app)
    , m_settings(app->settings())
    , m_library(app->library())
    , m_controller(app->controller())
    , m_playlists(app->playlists())
    , m_queue(app->queue())
    , m_scanner(app->scanner())
    , m_artwork(app->artwork())
    , m_lyrics(app->lyrics())
    , m_metadata(app->metadata())
    , m_sidebar(new QListWidget(this))
    , m_stack(new QStackedWidget(this))
    , m_playbackBar(new PlaybackBar(m_controller, m_artwork, this))
    , m_libraryPage(new LibraryPage(m_library, m_controller, m_playlists, this))
    , m_artistsPage(new ArtistsPage(m_library, m_controller, m_artwork, this))
    , m_albumsPage(new AlbumsPage(m_library, m_controller, m_artwork, this))
    , m_genresPage(new GenresPage(m_library, m_controller, m_artwork, this))
    , m_playlistsPage(new PlaylistsPage(m_playlists, m_controller, this))
    , m_queuePage(new QueuePage(m_queue, m_controller, m_library, this))
    , m_settingsPage(new SettingsPage(m_settings, m_scanner, this))
{
    setWindowTitle(tr("Phonio"));
    resize(1180, 760);
    setMinimumSize(980, 620);

    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("centralRoot"));
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    // --- Sidebar ---
    auto* sidebarFrame = new QFrame(this);
    sidebarFrame->setObjectName(QStringLiteral("sidebar"));
    sidebarFrame->setFixedWidth(210);
    auto* sidebarLayout = new QVBoxLayout(sidebarFrame);
    sidebarLayout->setContentsMargins(8, 16, 8, 16);
    sidebarLayout->setSpacing(12);

    auto* appTitle = new QLabel(tr("PHONIO"), sidebarFrame);
    appTitle->setObjectName(QStringLiteral("appTitle"));
    appTitle->setAlignment(Qt::AlignCenter);
    sidebarLayout->addWidget(appTitle);
    sidebarLayout->addSpacing(8);

    m_sidebar->setObjectName(QStringLiteral("sidebarList"));
    m_sidebar->setFrameShape(QFrame::NoFrame);
    m_sidebar->setSpacing(2);
    const QStringList items = { tr("Library"), tr("Artists"), tr("Albums"),
                                tr("Playlists"), tr("Genres"), tr("Queue"),
                                tr("Settings") };
    for (const QString& item : items)
        m_sidebar->addItem(item);
    sidebarLayout->addWidget(m_sidebar, 1);
    body->addWidget(sidebarFrame);

    // --- Pages ---
    m_stack->setObjectName(QStringLiteral("contentStack"));
    m_stack->addWidget(m_libraryPage);      // 0
    m_stack->addWidget(m_artistsPage);      // 1
    m_stack->addWidget(m_albumsPage);       // 2
    m_stack->addWidget(m_playlistsPage);    // 3
    m_stack->addWidget(m_genresPage);       // 4
    m_stack->addWidget(m_queuePage);        // 5
    m_stack->addWidget(m_settingsPage);     // 6
    body->addWidget(m_stack, 1);

    rootLayout->addLayout(body, 1);
    rootLayout->addWidget(m_playbackBar);
    setCentralWidget(central);

    statusBar()->showMessage(tr("Ready"));

    // --- Wiring: sidebar ---
    connect(m_sidebar, &QListWidget::currentRowChanged, this, &MainWindow::onSidebarChanged);
    m_sidebar->setCurrentRow(0);

    // --- Wiring: playback bar ---
    connect(m_playbackBar, &PlaybackBar::artworkClicked, this, &MainWindow::showNowPlaying);

    // --- Wiring: pages ---
    connect(m_libraryPage, &LibraryPage::editMetadataRequested, this, &MainWindow::handleEditMetadata);
    connect(m_libraryPage, &LibraryPage::attachLyricsRequested, this, &MainWindow::handleAttachLyrics);
    connect(m_playlistsPage, &PlaylistsPage::editMetadataRequested, this, &MainWindow::handleEditMetadata);
    connect(m_playlistsPage, &PlaylistsPage::attachLyricsRequested, this, &MainWindow::handleAttachLyrics);
    connect(m_queuePage, &QueuePage::editMetadataRequested, this, &MainWindow::handleEditMetadata);
    connect(m_queuePage, &QueuePage::attachLyricsRequested, this, &MainWindow::handleAttachLyrics);
    connect(m_settingsPage, &SettingsPage::rescanRequested, this, &MainWindow::ensureScanStarted);

    // --- Wiring: controller ---
    connect(m_controller, &PlaybackController::currentTrackChanged, this, &MainWindow::onCurrentTrackChanged);
    connect(m_controller, &PlaybackController::playbackStateChanged, this, &MainWindow::onPlaybackStateChanged);

    // --- Refresh browse pages ---
    connect(m_library, &LibraryManager::libraryChanged, this, [this] {
        m_artistsPage->setArtists(m_library->artists());
        m_albumsPage->setAlbums(m_library->albums());
        m_genresPage->setGenres(m_library->genres());
        statusBar()->showMessage(tr("%1 songs in library").arg(m_library->trackCount()), 4000);
    });
    m_artistsPage->setArtists(m_library->artists());
    m_albumsPage->setAlbums(m_library->albums());
    m_genresPage->setGenres(m_library->genres());

    ensureScanStarted();
}

void MainWindow::ensureScanStarted()
{
    if (m_scanStarted)
        return;
    m_scanStarted = true;
    m_scanner->setFolders(m_settings->musicFolders());
    m_scanner->start();
}

void MainWindow::onSidebarChanged(int row)
{
    if (row < 0 || row >= SidebarCount)
        return;
    m_browserStackIndex = row;
    m_stack->setCurrentIndex(row);
}

void MainWindow::showNowPlaying()
{
    if (!m_nowPlaying)
        return;
    m_stack->setCurrentWidget(m_nowPlaying);
    if (m_nowPlayingIndex < 0)
        m_nowPlayingIndex = m_stack->indexOf(m_nowPlaying);
    m_sidebar->clearSelection();
}

void MainWindow::setNowPlayingPage(NowPlayingWidget* page)
{
    m_nowPlaying = page;
    m_stack->addWidget(page);
    m_nowPlayingIndex = m_stack->indexOf(page);
    connect(m_nowPlaying, &NowPlayingWidget::backRequested, this, [this] {
        m_stack->setCurrentIndex(m_browserStackIndex);
        m_sidebar->setCurrentRow(m_browserStackIndex);
    });
}

void MainWindow::onCurrentTrackChanged(const Track& track)
{
    m_libraryPage->highlightTrack(track.id);
    m_queuePage->refreshTracks();
    statusBar()->showMessage(tr("Now playing: %1 - %2")
                                 .arg(track.displayTitle(), track.displayArtist()), 3000);
}

void MainWindow::onPlaybackStateChanged()
{
    // Title keeps the user informed even when minimized.
    if (const auto track = m_controller->currentTrack()) {
        const QString playing = m_controller->isPlaying() ? QStringLiteral("\u25B6 ")
                                                          : QStringLiteral("\u275A\u275A ");
        setWindowTitle(QStringLiteral("%1%2 - %3").arg(playing, track->displayTitle(), tr("Phonio")));
    }
}

void MainWindow::handleEditMetadata(const Track& track)
{
    MetadataDialog dialog(track, m_metadata, m_artwork, this);
    connect(&dialog, &MetadataDialog::metadataSaved, this, [this](const Track& updated) {
        m_library->applyMetadataRefresh(updated);
        m_artwork->invalidate(updated);
    });
    dialog.exec();
}

void MainWindow::handleAttachLyrics(const Track& track)
{
    const QString file = QFileDialog::getOpenFileName(
        this, tr("Attach Lyrics File"), QFileInfo(track.filePath).absolutePath(),
        tr("LRC files (*.lrc);;All files (*)"));
    if (file.isEmpty())
        return;
    const auto answer = QMessageBox::question(
        this, tr("Attach Lyrics"),
        tr("Copy the .lrc file next to the song, or store only a reference?\n\n"
           "Copying is recommended (lyrics travel with the file)."),
        tr("Copy Beside Song"), tr("Store Reference"), QString(), 0, 0);
    const bool copyBeside = (answer == 0);
    const QString result = m_lyrics->attachLyrics(track, file, copyBeside);
    if (result.isEmpty()) {
        QMessageBox::warning(this, tr("Attach Failed"), tr("Could not attach the lyrics file."));
        return;
    }
    if (m_controller->currentTrackId() == track.id)
        m_lyrics->loadLyricsFor(track);
}

} // namespace phonio
