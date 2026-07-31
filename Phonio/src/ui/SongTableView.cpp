#include "ui/SongTableView.h"

#include "ui/TrackListModel.h"
#include "playlists/PlaylistManager.h"

#include <QHeaderView>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDrag>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <functional>

namespace phonio {

namespace {
constexpr const char* kTrackMimeType = "application/x-phonio-track-ids";
constexpr int kRowHeight = 34;

bool lessThanTrackIds(const QByteArray& data, QVector<qint64>& out)
{
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
        return false;
    const auto array = doc.array();
    for (const auto& value : array)
        out.append(value.toDouble());
    return !out.isEmpty();
}
}

SongTableView::SongTableView(QWidget* parent)
    : QTableView(parent)
    , m_model(new TrackListModel(this))
    , m_proxy(new QSortFilterProxyModel(this))
{
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(-1);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setDynamicSortFilter(true);

    setModel(m_proxy);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAlternatingRowColors(true);
    setShowGrid(false);
    setWordWrap(false);
    setSortingEnabled(true);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setContextMenuPolicy(Qt::DefaultContextMenu);

    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(kRowHeight);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setHighlightSections(false);

    setItemDelegateForColumn(TrackListModel::RatingColumn, new RatingDelegate(this));

    setColumnWidth(TrackListModel::TitleColumn, 260);
    setColumnWidth(TrackListModel::ArtistColumn, 180);
    setColumnWidth(TrackListModel::AlbumColumn, 180);
    setColumnWidth(TrackListModel::GenreColumn, 110);
    setColumnWidth(TrackListModel::DurationColumn, 80);
    setColumnWidth(TrackListModel::YearColumn, 60);
    setColumnWidth(TrackListModel::RatingColumn, 90);

    connect(this, &QTableView::doubleClicked, this, [this](const QModelIndex& index) {
        const QModelIndex source = m_proxy->mapToSource(index);
        if (const auto track = m_model->trackAt(source.row()))
            emit doubleClickedTrack(*track);
    });
}

void SongTableView::setTracks(const QVector<Track>& tracks)
{
    m_model->setTracks(tracks);
}

const QVector<Track>& SongTableView::tracks() const
{
    return m_model->tracks();
}

void SongTableView::updateTrack(const Track& track)
{
    m_model->updateTrack(track);
}

void SongTableView::removeTrackById(qint64 trackId)
{
    m_model->removeTrack(trackId);
}

void SongTableView::setFilterText(const QString& text)
{
    m_proxy->setFilterFixedString(text.trimmed());
}

bool SongTableView::isFiltering() const
{
    return !m_proxy->filterRegularExpression().pattern().isEmpty();
}

void SongTableView::setPlaylistManager(PlaylistManager* playlists)
{
    m_playlists = playlists;
}

QVector<Track> SongTableView::selectedTracks() const
{
    QVector<Track> result;
    const auto rows = selectionModel()->selectedRows();
    for (const auto& index : rows) {
        const QModelIndex source = m_proxy->mapToSource(index);
        if (const auto track = m_model->trackAt(source.row()))
            result.append(*track);
    }
    return result;
}

QVector<Track> SongTableView::filteredTracks() const
{
    QVector<Track> result;
    result.reserve(m_proxy->rowCount());
    for (int row = 0; row < m_proxy->rowCount(); ++row) {
        const QModelIndex source = m_proxy->mapToSource(m_proxy->index(row, 0));
        if (const auto track = m_model->trackAt(source.row()))
            result.append(*track);
    }
    return result;
}

Track SongTableView::trackAtRow(int row) const
{
    const QModelIndex proxyIndex = m_proxy->index(row, 0);
    if (!proxyIndex.isValid())
        return {};
    const QModelIndex source = m_proxy->mapToSource(proxyIndex);
    const auto track = m_model->trackAt(source.row());
    return track.value_or(Track{});
}

void SongTableView::highlightTrackId(qint64 trackId)
{
    m_highlightedId = trackId;
    const int sourceRow = m_model->rowOfTrackId(trackId);
    if (sourceRow >= 0) {
        const QModelIndex proxy = m_proxy->mapFromSource(m_model->index(sourceRow, 0));
        if (proxy.isValid())
            selectionModel()->select(proxy, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
    m_proxy->invalidate(); // repaint for delegate highlight
}

void SongTableView::clearHighlight()
{
    m_highlightedId = -1;
    m_proxy->invalidate();
}

void SongTableView::setAcceptTrackDrops(bool enabled)
{
    m_acceptTrackDrops = enabled;
}

void SongTableView::mouseDoubleClickEvent(QMouseEvent* event)
{
    QTableView::mouseDoubleClickEvent(event);
    const QModelIndex index = indexAt(event->pos());
    if (!index.isValid())
        return;
    const QModelIndex source = m_proxy->mapToSource(index);
    if (const auto track = m_model->trackAt(source.row()))
        emit doubleClickedTrack(*track);
}

void SongTableView::contextMenuEvent(QContextMenuEvent* event)
{
    const QModelIndex index = indexAt(event->pos());
    QVector<Track> tracks = selectedTracks();
    Track anchor;
    if (index.isValid()) {
        const QModelIndex source = m_proxy->mapToSource(index);
        const auto track = m_model->trackAt(source.row());
        if (track)
            anchor = *track;
    }
    if (tracks.isEmpty() && anchor.isValid())
        tracks.append(anchor);
    if (tracks.isEmpty())
        return;
    buildContextMenu(event->globalPos(), anchor, tracks);
}

void SongTableView::buildContextMenu(const QPoint& globalPos, const Track& anchor, const QVector<Track>& tracks)
{
    QMenu menu(this);

    menu.addAction(tr("Play"), this, [this, anchor] { emit playRequested(anchor); });
    menu.addAction(tr("Play Next"), this, [this, tracks] { emit playNextRequested(tracks); });
    menu.addAction(tr("Add to Queue"), this, [this, tracks] { emit addToQueueRequested(tracks); });

    if (m_playlists) {
        auto* playlistMenu = menu.addMenu(tr("Add to Playlist"));
        const bool hasPlaylists = !m_playlists->playlists().isEmpty();
        if (hasPlaylists) {
            for (const auto& info : m_playlists->playlists())
                playlistMenu->addAction(info.name, this, [this, id = info.id, tracks] {
                    emit addToPlaylistRequested(id, tracks);
                });
        } else {
            playlistMenu->addAction(tr("No playlists yet"))->setEnabled(false);
        }
    }

    menu.addSeparator();
    menu.addAction(tr("Edit Metadata..."), this, [this, anchor] { emit editMetadataRequested(anchor); });
    menu.addAction(tr("Add Lyrics..."), this, [this, anchor] { emit attachLyricsRequested(anchor); });

    auto* ratingMenu = menu.addMenu(tr("Rating"));
    for (int stars = 1; stars <= 5; ++stars) {
        const int value = stars;
        ratingMenu->addAction(QString(stars, QLatin1Char('★')), this,
                              [this, anchor, value] { emit setRatingRequested(anchor.id, value); });
    }
    ratingMenu->addAction(tr("Clear Rating"), this, [this, anchor] { emit setRatingRequested(anchor.id, 0); });

    menu.addAction(anchor.favorite ? tr("Remove from Favorites") : tr("Add to Favorites"), this,
                   [this, anchor] { emit toggleFavoriteRequested(anchor.id); });
    menu.addSeparator();
    menu.addAction(tr("Show in File Manager"), this, [this, anchor] {
        emit showInFileManagerRequested(anchor.filePath);
    });
    menu.addAction(tr("Remove from Library"), this, [this, anchor] { emit removeTrackRequested(anchor.id); });

    menu.exec(globalPos);
}

void SongTableView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat(QLatin1String(kTrackMimeType)))
        event->acceptProposedAction();
    else
        QTableView::dragEnterEvent(event);
}

void SongTableView::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat(QLatin1String(kTrackMimeType)))
        event->acceptProposedAction();
    else
        QTableView::dragMoveEvent(event);
}

void SongTableView::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasFormat(QLatin1String(kTrackMimeType))) {
        QTableView::dropEvent(event);
        return;
    }
    QVector<qint64> ids;
    if (!lessThanTrackIds(event->mimeData()->data(QLatin1String(kTrackMimeType)), ids)) {
        event->ignore();
        return;
    }

    // Internal reorder
    if (event->source() == this) {
        const QModelIndex dropIndex = indexAt(event->position().toPoint());
        const int dropRow = dropIndex.isValid() ? m_proxy->mapToSource(dropIndex).row() : m_model->rowCount() - 1;
        if (onInternalDrop && m_dragStartRow >= 0 && dropRow >= 0) {
            onInternalDrop(m_dragStartRow, dropRow);
            event->acceptProposedAction();
        }
        return;
    }

    // External drop from library/playlists
    if (m_acceptTrackDrops) {
        QVector<Track> dropped;
        for (qint64 id : ids) {
            const int row = m_model->rowOfTrackId(id);
            if (row >= 0) {
                if (const auto track = m_model->trackAt(row))
                    dropped.append(*track);
            }
        }
        if (!dropped.isEmpty()) {
            emit tracksDropped(dropped);
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void SongTableView::startDrag(Qt::DropActions supportedActions)
{
    const auto rows = selectionModel()->selectedRows();
    if (rows.isEmpty())
        return;
    m_dragStartRow = m_proxy->mapToSource(rows.first()).row();

    QVector<qint64> ids;
    for (const auto& index : rows) {
        const QModelIndex source = m_proxy->mapToSource(index);
        if (const auto track = m_model->trackAt(source.row()))
            ids.append(track->id);
    }
    auto* mime = new QMimeData;
    QJsonArray array;
    for (qint64 id : ids)
        array.append(double(id));
    mime->setData(QLatin1String(kTrackMimeType), QJsonDocument(array).toJson());

    auto* drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->exec(supportedActions, Qt::MoveAction);
    delete drag;
}

// ---------------------------------------------------------------------------
// RatingDelegate
// ---------------------------------------------------------------------------

void RatingDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                           const QModelIndex& index) const
{
    const int rating = index.data(Qt::DisplayRole).toInt();
    if (rating <= 0 || index.column() != TrackListModel::RatingColumn) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());
    painter->setPen(QColor(200, 200, 200));
    QFont font = option.font;
    font.setPixelSize(13);
    painter->setFont(font);
    painter->drawText(option.rect.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignRight,
                      QString(rating, QLatin1Char('★')));
    painter->restore();
}

} // namespace phonio
