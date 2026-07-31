#pragma once

#include "core/Types.h"

#include <QObject>
#include <QVector>
#include <QHash>
#include <optional>

class QSqlDatabase;

namespace phonio {

// Owns the SQLite database and all persistence.
// Thread-safe only for the "tracks" table via dedicated methods documented below;
// playlist/queue tables are manipulated on the main thread.
class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(const QString& databasePath, QObject* parent = nullptr);
    ~DatabaseManager() override;

    QString databasePath() const { return m_databasePath; }

    // --- Library -------------------------------------------------------
    QVector<Track> loadAllTracks() const;
    std::optional<Track> trackById(qint64 id) const;
    std::optional<Track> trackByPath(const QString& filePath) const;

    // May be called from a worker thread (uses its own connection).
    qint64 upsertTrack(const Track& track);
    void updateTrackStats(const Track& track);          // rating/playCount/lastPlayed/favorite
    void updateTrackLyricsPath(qint64 trackId, const QString& lyricsPath);
    void removeTracksNotIn(const QSet<QString>& existingPaths);
    void removeTrack(qint64 trackId);

    // --- Playlists -----------------------------------------------------
    QVector<PlaylistInfo> loadPlaylists() const;
    qint64 createPlaylist(const QString& name);
    bool renamePlaylist(qint64 playlistId, const QString& newName);
    void deletePlaylist(qint64 playlistId);
    QVector<qint64> playlistTrackIds(qint64 playlistId) const;
    void setPlaylistTracks(qint64 playlistId, const QVector<qint64>& trackIds);

    // --- Queue ---------------------------------------------------------
    QVector<qint64> loadQueue() const;
    void saveQueue(const QVector<qint64>& trackIds);

private:
    void open();
    void migrate(int fromVersion);

    QString m_databasePath;
    QString m_mainConnection;
};

} // namespace phonio
