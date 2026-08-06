#include "ui/TrackListModel.h"

#include "core/Types.h"

namespace phonio {

TrackListModel::TrackListModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

void TrackListModel::setTracks(const QVector<Track>& tracks)
{
    beginResetModel();
    m_tracks = tracks;
    endResetModel();
}

void TrackListModel::updateTrack(const Track& track)
{
    const int row = rowOfTrackId(track.id);
    if (row < 0)
        return;
    m_tracks[row] = track;
    emit dataChanged(index(row, 0), index(row, ColumnCount - 1));
}

void TrackListModel::removeTrack(qint64 trackId)
{
    const int row = rowOfTrackId(trackId);
    if (row < 0)
        return;
    beginRemoveRows({}, row, row);
    m_tracks.removeAt(row);
    endRemoveRows();
}

void TrackListModel::clear()
{
    beginResetModel();
    m_tracks.clear();
    endResetModel();
}

std::optional<Track> TrackListModel::trackAt(int row) const
{
    if (row < 0 || row >= m_tracks.size())
        return std::nullopt;
    return m_tracks.at(row);
}

int TrackListModel::rowOfTrackId(qint64 id) const
{
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks.at(i).id == id)
            return i;
    }
    return -1;
}

int TrackListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_tracks.size();
}

int TrackListModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant TrackListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tracks.size())
        return {};
    const Track& track = m_tracks.at(index.row());

    switch (role) {
    case TrackRole:
        return QVariant::fromValue(track);
    case TrackIdRole:
        return track.id;
    case DurationMsRole:
        return track.durationMs;
    case RatingRole:
        return track.rating;
    case Qt::DisplayRole:
        switch (index.column()) {
        case TitleColumn: return track.displayTitle();
        case ArtistColumn: return track.displayArtist();
        case AlbumColumn: return track.displayAlbum();
        case GenreColumn: return track.displayGenre();
        case DurationColumn: return formatDurationMs(track.durationMs);
        case YearColumn: return track.year > 0 ? track.year : QVariant();
        case RatingColumn: return track.rating > 0 ? track.rating : QVariant();
        }
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == DurationColumn || index.column() == YearColumn || index.column() == RatingColumn)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        break;
    case Qt::ToolTipRole:
        return track.filePath;
    }
    return {};
}

QVariant TrackListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case TitleColumn: return tr("Title");
    case ArtistColumn: return tr("Artist");
    case AlbumColumn: return tr("Album");
    case GenreColumn: return tr("Genre");
    case DurationColumn: return tr("Duration");
    case YearColumn: return tr("Year");
    case RatingColumn: return tr("Rating");
    }
    return {};
}

} // namespace phonio
