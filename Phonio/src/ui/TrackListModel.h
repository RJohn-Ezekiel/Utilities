#pragma once

#include "core/Types.h"

#include <QAbstractTableModel>
#include <QVector>

namespace phonio {

// Table model over an ordered list of tracks. Used by the library page,
// playlist page and queue page. Column set is uniform.
class TrackListModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        TitleColumn,
        ArtistColumn,
        AlbumColumn,
        GenreColumn,
        DurationColumn,
        YearColumn,
        RatingColumn,
        ColumnCount
    };

    enum Roles {
        TrackRole = Qt::UserRole + 1,   // full phonio::Track
        TrackIdRole,
        DurationMsRole,
        RatingRole,
    };

    explicit TrackListModel(QObject* parent = nullptr);

    void setTracks(const QVector<Track>& tracks);
    void updateTrack(const Track& track);
    void removeTrack(qint64 trackId);
    void clear();

    const QVector<Track>& tracks() const { return m_tracks; }
    std::optional<Track> trackAt(int row) const;
    int rowOfTrackId(qint64 id) const;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QVector<Track> m_tracks;
};

} // namespace phonio
