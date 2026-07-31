#include "ui/LibraryPage.h"

#include "ui/SongTableView.h"
#include "library/LibraryManager.h"
#include "player/PlaybackController.h"
#include "playlists/PlaylistManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

namespace phonio {

LibraryPage::LibraryPage(LibraryManager* library, PlaybackController* controller,
                         PlaylistManager* playlists, QWidget* parent)
    : QWidget(parent)
    , m_library(library)
    , m_controller(controller)
    , m_table(new SongTableView(this))
    , m_searchBox(new QLineEdit(this))
    , m_sortBox(new QComboBox(this))
    , m_countLabel(new QLabel(this))
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 16);
    root->setSpacing(14);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel(tr("Library"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    m_countLabel->setObjectName(QStringLiteral("pageSubtitle"));
    titleRow->addWidget(title);
    titleRow->addSpacing(10);
    titleRow->addWidget(m_countLabel, 0, Qt::AlignBottom);
    titleRow->addStretch();
    root->addLayout(titleRow);

    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(10);
    m_searchBox->setPlaceholderText(tr("Search songs, artists, albums, genres..."));
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setFixedWidth(340);

    m_sortBox->addItems({tr("Title"), tr("Artist"), tr("Album"), tr("Genre"),
                         tr("Duration"), tr("Year"), tr("Rating")});
    m_sortBox->setCurrentIndex(0);

    auto* playAllButton = new QPushButton(tr("Play All"), this);
    playAllButton->setObjectName(QStringLiteral("accentButton"));

    toolbar->addWidget(m_searchBox);
    toolbar->addStretch();
    toolbar->addWidget(new QLabel(tr("Sort by"), this));
    toolbar->addWidget(m_sortBox);
    toolbar->addWidget(playAllButton);
    root->addLayout(toolbar);

    m_table->setPlaylistManager(playlists);
    root->addWidget(m_table, 1);

    // --- Search ---
    connect(m_searchBox, &QLineEdit::textChanged, m_table, &SongTableView::setFilterText);

    // --- Sorting ---
    connect(m_sortBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_table->sortByColumn(index, Qt::AscendingOrder);
    });

    // --- Playback wiring ---
    connect(m_table, &SongTableView::doubleClickedTrack, this,
            [this](const Track& track) { m_controller->playTrack(track); });
    connect(m_table, &SongTableView::playRequested, this,
            [this](const Track& track) { m_controller->playTrack(track); });
    connect(m_table, &SongTableView::playNextRequested, this,
            [this](const QVector<Track>& tracks) {
                QVector<qint64> ids;
                for (const auto& t : tracks)
                    ids.append(t.id);
                m_controller->queue()->enqueueNext(ids);
            });
    connect(m_table, &SongTableView::addToQueueRequested, this,
            [this](const QVector<Track>& tracks) {
                QVector<qint64> ids;
                for (const auto& t : tracks)
                    ids.append(t.id);
                m_controller->queue()->enqueue(ids);
            });
    connect(m_table, &SongTableView::addToPlaylistRequested, this,
            [this, playlists](qint64 playlistId, const QVector<Track>& tracks) {
                QVector<qint64> ids;
                for (const auto& t : tracks)
                    ids.append(t.id);
                playlists->appendTracks(playlistId, ids);
            });
    connect(m_table, &SongTableView::editMetadataRequested, this,
            &LibraryPage::editMetadataRequested);
    connect(m_table, &SongTableView::attachLyricsRequested, this,
            &LibraryPage::attachLyricsRequested);
    connect(m_table, &SongTableView::setRatingRequested, this,
            [this](qint64 id, int rating) {
                if (const auto track = m_library->trackById(id))
                    m_library->setRating(*track, rating);
            });
    connect(m_table, &SongTableView::toggleFavoriteRequested, this,
            [this](qint64 id) {
                if (const auto track = m_library->trackById(id))
                    m_library->setFavorite(*track, !track->favorite);
            });
    connect(m_table, &SongTableView::removeTrackRequested, this,
            [this](qint64 id) { m_library->removeTrack(id); });
    connect(m_table, &SongTableView::showInFileManagerRequested, this,
            [](const QString& filePath) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
            });

    connect(playAllButton, &QPushButton::clicked, this, [this] {
        const QVector<Track> tracks = m_table->isFiltering() ? m_table->filteredTracks()
                                                            : m_library->tracks();
        if (!tracks.isEmpty())
            m_controller->playTracks(tracks, 0);
    });

    // --- Library signals ---
    connect(m_library, &LibraryManager::libraryChanged, this, &LibraryPage::onLibraryChanged);
    connect(m_library, &LibraryManager::trackChanged, this, &LibraryPage::onTrackChanged);
    connect(m_library, &LibraryManager::trackRemoved, this, &LibraryPage::onTrackRemoved);

    refresh();
}

void LibraryPage::refresh()
{
    m_table->setTracks(m_library->tracks());
    m_countLabel->setText(tr("%1 songs").arg(m_library->trackCount()));
    if (const auto current = m_controller->currentTrack())
        m_table->highlightTrackId(current->id);
    else
        m_table->clearHighlight();
}

void LibraryPage::highlightTrack(qint64 trackId)
{
    m_table->highlightTrackId(trackId);
}

void LibraryPage::onLibraryChanged()
{
    refresh();
}

void LibraryPage::onTrackChanged(qint64 trackId)
{
    if (const auto track = m_library->trackById(trackId))
        m_table->updateTrack(*track);
}

void LibraryPage::onTrackRemoved(qint64 trackId)
{
    m_table->removeTrackById(trackId);
}

} // namespace phonio
