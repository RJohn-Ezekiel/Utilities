#pragma once

#include "core/Types.h"

#include <QObject>
#include <QVector>
#include <QHash>

namespace phonio {

class DatabaseManager;

// In-memory model of the whole music library.
// Owns track list and derived artist/album/genre groupings; persists
// stats (rating, play count, favorites, lyrics path) through DatabaseManager.
class LibraryManager : public QObject
{
    Q_OBJECT

public:
    explicit LibraryManager(DatabaseManager* db, QObject* parent = nullptr);

    void reloadFromDatabase();

    const QVector<Track>& tracks() const { return m_tracks; }
    int trackCount() const { return m_tracks.size(); }
    std::optional<Track> trackById(qint64 id) const;
    Track trackByRow(int row) const;
    int rowOfTrackId(qint64 id) const;

    const QVector<Artist>& artists() const { return m_artists; }
    const QVector<Album>& albums() const { return m_albums; }
    const QVector<Genre>& genres() const { return m_genres; }

    // Mutations (persist + notify)
    void updateTrack(const Track& updated);                 // replace in place
    void incrementPlayCount(const Track& track);
    void setFavorite(const Track& track, bool favorite);
    void setRating(const Track& track, int rating);
    void setLyricsPath(qint64 trackId, const QString& path);
    void removeTrack(qint64 trackId);
    void applyMetadataRefresh(const Track& updated);        // after tag edits

    // Lookup helpers
    QVector<Track> tracksForIds(const QVector<qint64>& ids) const;

signals:
    void libraryChanged();
    void trackChanged(qint64 trackId);
    void trackRemoved(qint64 trackId);
    void tracksAdded(int count);

private:
    void rebuildGroupings();
    void updateTrackInternal(const Track& updated);

    DatabaseManager* m_db;
    QVector<Track> m_tracks;
    QVector<Artist> m_artists;
    QVector<Album> m_albums;
    QVector<Genre> m_genres;
};

} // namespace phonio
