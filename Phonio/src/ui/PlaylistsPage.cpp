#include "ui/PlaylistsPage.h"

#include "ui/SongTableView.h"
#include "playlists/PlaylistManager.h"
#include "player/PlaybackController.h"

#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

namespace phonio {

PlaylistsPage::PlaylistsPage(PlaylistManager* playlists, PlaybackController* controller, QWidget* parent)
    : QWidget(parent)
    , m_playlists(playlists)
    , m_controller(controller)
    , m_nav(new QListWidget(this))
    , m_table(new SongTableView(this))
    , m_title(new QLabel(this))
    , m_countLabel(new QLabel(this))
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 16);
    root->setSpacing(14);

    m_title->setObjectName(QStringLiteral("pageTitle"));
    m_title->setText(tr("Playlists"));
    auto* titleRow = new QHBoxLayout;
    titleRow->addWidget(m_title);
    titleRow->addStretch();
    root->addLayout(titleRow);

    auto* body = new QHBoxLayout;
    body->setSpacing(16);

    // Left: navigation
    auto* leftPanel = new QWidget(this);
    leftPanel->setFixedWidth(230);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    m_nav->setObjectName(QStringLiteral("playlistNav"));

    auto* newButton = new QPushButton(tr("New Playlist"), this);
    newButton->setObjectName(QStringLiteral("accentButton"));

    leftLayout->addWidget(newButton);
    leftLayout->addWidget(m_nav, 1);
    body->addWidget(leftPanel);

    // Right: tracks of selected playlist
    auto* rightPanel = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto* headerRow = new QHBoxLayout;
    m_countLabel->setObjectName(QStringLiteral("pageSubtitle"));
    auto* playButton = new QPushButton(tr("Play"), this);
    playButton->setObjectName(QStringLiteral("accentButton"));
    headerRow->addWidget(m_countLabel);
    headerRow->addStretch();
    headerRow->addWidget(playButton);
    rightLayout->addLayout(headerRow);

    m_table->setPlaylistManager(playlists);
    m_table->setAcceptTrackDrops(true);
    rightLayout->addWidget(m_table, 1);
    body->addWidget(rightPanel, 1);

    root->addLayout(body, 1);

    // --- Wiring: navigation ---
    connect(newButton, &QPushButton::clicked, this, [this] {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("New Playlist"),
                                                   tr("Playlist name:"), QLineEdit::Normal,
                                                   QString(), &ok);
        if (ok && !name.trimmed().isEmpty())
            selectPlaylist(m_playlists->createPlaylist(name.trimmed()));
    });
    connect(m_nav, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) {
        if (current)
            loadPlaylist(current->data(Qt::UserRole).toLongLong());
    });
    connect(m_nav, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QListWidgetItem* item = m_nav->itemAt(pos);
        if (!item)
            return;
        const qint64 id = item->data(Qt::UserRole).toLongLong();
        QMenu menu(this);
        menu.addAction(tr("Rename"), this, [this, id] {
            bool ok = false;
            const QString name = QInputDialog::getText(this, tr("Rename Playlist"),
                                                       tr("Playlist name:"), QLineEdit::Normal,
                                                       m_playlists->playlistById(id)->name, &ok);
            if (ok && !name.trimmed().isEmpty())
                m_playlists->renamePlaylist(id, name.trimmed());
        });
        menu.addAction(tr("Delete"), this, [this, id] {
            const auto answer = QMessageBox::question(this, tr("Delete Playlist"),
                                                      tr("Delete this playlist?"));
            if (answer == QMessageBox::Yes)
                m_playlists->deletePlaylist(id);
        });
        menu.exec(m_nav->viewport()->mapToGlobal(pos));
    });
    m_nav->setContextMenuPolicy(Qt::CustomContextMenu);

    // --- Wiring: tracks ---
    connect(playButton, &QPushButton::clicked, this, [this] {
        if (m_selectedPlaylistId >= 0 && !m_table->tracks().isEmpty())
            m_controller->playTracks(m_table->tracks(), 0);
    });
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
            [this](qint64 targetId, const QVector<Track>& tracks) {
                QVector<qint64> ids;
                for (const auto& t : tracks)
                    ids.append(t.id);
                m_playlists->appendTracks(targetId, ids);
            });
    connect(m_table, &SongTableView::editMetadataRequested, this, &PlaylistsPage::editMetadataRequested);
    connect(m_table, &SongTableView::attachLyricsRequested, this, &PlaylistsPage::attachLyricsRequested);
    connect(m_table, &SongTableView::removeTrackRequested, this, [this](qint64 trackId) {
        if (m_selectedPlaylistId >= 0)
            m_playlists->removeTrackFromPlaylist(m_selectedPlaylistId, trackId);
    });
    connect(m_table, &SongTableView::showInFileManagerRequested, this,
            [](const QString& filePath) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
            });

    // Drag from library into the playlist table
    connect(m_table, &SongTableView::tracksDropped, this, [this](const QVector<Track>& tracks) {
        if (m_selectedPlaylistId < 0)
            return;
        QVector<qint64> ids;
        for (const auto& t : tracks)
            ids.append(t.id);
        m_playlists->appendTracks(m_selectedPlaylistId, ids);
    });

    connect(m_playlists, &PlaylistManager::playlistsChanged, this, &PlaylistsPage::onPlaylistsChanged);
    connect(m_playlists, &PlaylistManager::playlistContentChanged, this, &PlaylistsPage::onPlaylistContentChanged);

    onPlaylistsChanged();
}

void PlaylistsPage::onPlaylistsChanged()
{
    const qint64 previouslySelected = m_selectedPlaylistId;
    m_nav->blockSignals(true);
    m_nav->clear();
    for (const auto& info : m_playlists->playlists()) {
        auto* item = new QListWidgetItem(
            QStringLiteral("%1  (%2)").arg(info.name).arg(info.trackCount), m_nav);
        item->setData(Qt::UserRole, info.id);
        if (info.id == previouslySelected)
            m_nav->setCurrentItem(item);
    }
    m_nav->blockSignals(false);
    if (m_nav->currentItem())
        loadPlaylist(m_nav->currentItem()->data(Qt::UserRole).toLongLong());
}

void PlaylistsPage::onPlaylistContentChanged(qint64 playlistId)
{
    if (playlistId == m_selectedPlaylistId)
        m_table->setTracks(m_playlists->tracksOf(playlistId));
    onPlaylistsChanged();
}

void PlaylistsPage::selectPlaylist(qint64 playlistId)
{
    for (int i = 0; i < m_nav->count(); ++i) {
        if (m_nav->item(i)->data(Qt::UserRole).toLongLong() == playlistId) {
            m_nav->setCurrentRow(i);
            return;
        }
    }
}

void PlaylistsPage::loadPlaylist(qint64 playlistId)
{
    m_selectedPlaylistId = playlistId;
    m_table->setTracks(m_playlists->tracksOf(playlistId));
    m_countLabel->setText(tr("%1 songs").arg(m_table->tracks().size()));
    const auto info = m_playlists->playlistById(playlistId);
    m_title->setText(info ? info->name : tr("Playlists"));
}

} // namespace phonio
