#include "ui/QueuePage.h"

#include "ui/SongTableView.h"
#include "queue/QueueManager.h"
#include "player/PlaybackController.h"
#include "library/LibraryManager.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <algorithm>
#include <functional>

namespace phonio {

QueuePage::QueuePage(QueueManager* queue, PlaybackController* controller,
                     LibraryManager* library, QWidget* parent)
    : QWidget(parent)
    , m_queue(queue)
    , m_controller(controller)
    , m_library(library)
    , m_table(new SongTableView(this))
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 16);
    root->setSpacing(14);

    auto* title = new QLabel(tr("Queue"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    auto* titleRow = new QHBoxLayout;
    titleRow->addWidget(title);
    titleRow->addStretch();
    root->addLayout(titleRow);

    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);
    auto* removeButton = new QPushButton(tr("Remove Selected"), this);
    auto* upButton = new QPushButton(tr("Move Up"), this);
    auto* downButton = new QPushButton(tr("Move Down"), this);
    auto* clearButton = new QPushButton(tr("Clear"), this);
    toolbar->addWidget(removeButton);
    toolbar->addWidget(upButton);
    toolbar->addWidget(downButton);
    toolbar->addStretch();
    toolbar->addWidget(clearButton);
    root->addLayout(toolbar);

    m_table->setAcceptTrackDrops(true);
    root->addWidget(m_table, 1);

    // --- Controls ---
    connect(removeButton, &QPushButton::clicked, this, [this] {
        const auto rows = m_table->selectionModel()->selectedRows();
        QVector<int> sourceRows;
        for (const auto& index : rows) {
            const Track track = m_table->trackAtRow(index.row());
            if (track.isValid())
                sourceRows.append(m_table->tracks().indexOf(track));
        }
        std::sort(sourceRows.begin(), sourceRows.end(), std::greater<int>());
        for (int row : sourceRows)
            m_queue->removeAt(row);
    });
    connect(upButton, &QPushButton::clicked, this, [this] {
        const auto rows = m_table->selectionModel()->selectedRows();
        if (rows.isEmpty())
            return;
        const int row = m_table->tracks().indexOf(m_table->trackAtRow(rows.first().row()));
        m_queue->moveUp(row);
    });
    connect(downButton, &QPushButton::clicked, this, [this] {
        const auto rows = m_table->selectionModel()->selectedRows();
        if (rows.isEmpty())
            return;
        const int row = m_table->tracks().indexOf(m_table->trackAtRow(rows.first().row()));
        m_queue->moveDown(row);
    });
    connect(clearButton, &QPushButton::clicked, this, [this] {
        m_queue->clear();
        m_controller->player()->stop();
    });

    // --- Table actions ---
    connect(m_table, &SongTableView::doubleClickedTrack, this,
            [this](const Track& track) { m_controller->playTrack(track); });
    connect(m_table, &SongTableView::playRequested, this,
            [this](const Track& track) { m_controller->playTrack(track); });
    connect(m_table, &SongTableView::addToQueueRequested, this,
            [this](const QVector<Track>& tracks) {
                QVector<qint64> ids;
                for (const auto& t : tracks)
                    ids.append(t.id);
                m_queue->enqueue(ids);
            });
    connect(m_table, &SongTableView::playNextRequested, this,
            [this](const QVector<Track>& tracks) {
                QVector<qint64> ids;
                for (const auto& t : tracks)
                    ids.append(t.id);
                m_queue->enqueueNext(ids);
            });
    connect(m_table, &SongTableView::removeTrackRequested, this, [this](qint64 trackId) {
        const int row = m_table->tracks().indexOf(m_library->trackById(trackId).value_or(Track{}));
        if (row >= 0)
            m_queue->removeAt(row);
    });
    connect(m_table, &SongTableView::editMetadataRequested, this, &QueuePage::editMetadataRequested);
    connect(m_table, &SongTableView::attachLyricsRequested, this, &QueuePage::attachLyricsRequested);

    // Internal drag reorder
    m_table->onInternalDrop = [this](int fromRow, int toRow) {
        m_queue->move(fromRow, toRow);
    };

    // --- Signals ---
    connect(m_queue, &QueueManager::queueChanged, this, &QueuePage::onQueueChanged);
    connect(m_controller, &PlaybackController::currentTrackChanged, this, &QueuePage::onCurrentTrackChanged);
    connect(m_library, &LibraryManager::trackChanged, this, &QueuePage::onLibraryTrackChanged);

    onQueueChanged();
}

void QueuePage::refreshTracks()
{
    const QVector<Track> tracks = m_library->tracksForIds(m_queue->trackIds());
    m_table->setTracks(tracks);
    if (const auto current = m_controller->currentTrack())
        m_table->highlightTrackId(current->id);
}

void QueuePage::onQueueChanged()
{
    refreshTracks();
}

void QueuePage::onCurrentTrackChanged(const Track& track)
{
    m_table->highlightTrackId(track.id);
}

void QueuePage::onLibraryTrackChanged(qint64 trackId)
{
    const int row = m_table->tracks().indexOf(m_library->trackById(trackId).value_or(Track{}));
    if (row >= 0)
        m_table->updateTrack(m_library->trackById(trackId).value());
}

} // namespace phonio
