#pragma once

#include "core/Types.h"

#include <QTableView>
#include <QVector>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>

class QMenu;
class QStyledItemDelegate;

namespace phonio {

class TrackListModel;
class PlaylistManager;
class ArtworkManager;

// Reusable track table with search filtering, sorting, context menu and
// drag-out support. Pages compose it with the managers they own.
class SongTableView : public QTableView
{
    Q_OBJECT

public:
    explicit SongTableView(QWidget* parent = nullptr);

    void setTracks(const QVector<Track>& tracks);
    const QVector<Track>& tracks() const;
    void updateTrack(const Track& track);
    void removeTrackById(qint64 trackId);

    void setFilterText(const QString& text);
    void setPlaylistManager(PlaylistManager* playlists);

    QVector<Track> selectedTracks() const;
    QVector<Track> filteredTracks() const;      // visible rows after search filter
    Track trackAtRow(int row) const;

    void highlightTrackId(qint64 trackId);
    void clearHighlight();
    qint64 highlightedTrackId() const { return m_highlightedId; }

    // Accepts internal drop (reorder) or external drops of track ids from the library.
    void setAcceptTrackDrops(bool enabled);
    std::function<void(int fromRow, int toRow)> onInternalDrop;

    bool isFiltering() const;

signals:
    void playRequested(const Track& track);
    void doubleClickedTrack(const Track& track);
    void addToQueueRequested(const QVector<Track>& tracks);
    void playNextRequested(const QVector<Track>& tracks);
    void addToPlaylistRequested(qint64 playlistId, const QVector<Track>& tracks);
    void editMetadataRequested(const Track& track);
    void attachLyricsRequested(const Track& track);
    void setRatingRequested(qint64 trackId, int rating);
    void toggleFavoriteRequested(qint64 trackId);
    void removeTrackRequested(qint64 trackId);
    void showInFileManagerRequested(const QString& filePath);
    void tracksDropped(const QVector<Track>& tracks);   // external drop accepted

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

private:
    void buildContextMenu(const QPoint& globalPos, const Track& track, const QVector<Track>& tracks);

    TrackListModel* m_model;
    QSortFilterProxyModel* m_proxy;
    PlaylistManager* m_playlists = nullptr;
    qint64 m_highlightedId = -1;
    bool m_acceptTrackDrops = false;
    int m_dragStartRow = -1;
};

// Delegate drawing the rating column as stars.
class RatingDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

} // namespace phonio
