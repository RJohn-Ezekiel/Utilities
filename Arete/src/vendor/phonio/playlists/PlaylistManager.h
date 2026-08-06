#pragma once

#include "core/Types.h"

#include <QObject>
#include <QVector>

namespace phonio {

class DatabaseManager;
class LibraryManager;

// CRUD for user playlists; track membership persisted as ordered ids.
class PlaylistManager : public QObject
{
    Q_OBJECT

public:
    explicit PlaylistManager(DatabaseManager* db, LibraryManager* library, QObject* parent = nullptr);

    void reload();
    const QVector<PlaylistInfo>& playlists() const { return m_playlists; }
    std::optional<PlaylistInfo> playlistById(qint64 id) const;

    qint64 createPlaylist(const QString& name);
    bool renamePlaylist(qint64 playlistId, const QString& newName);
    void deletePlaylist(qint64 playlistId);

    QVector<Track> tracksOf(qint64 playlistId) const;
    QVector<qint64> trackIdsOf(qint64 playlistId) const;

    void setTracks(qint64 playlistId, const QVector<qint64>& trackIds);
    void appendTracks(qint64 playlistId, const QVector<qint64>& trackIds);
    void removeTrackFromPlaylist(qint64 playlistId, qint64 trackId);

signals:
    void playlistsChanged();
    void playlistContentChanged(qint64 playlistId);

private:
    DatabaseManager* m_db;
    LibraryManager* m_library;
    QVector<PlaylistInfo> m_playlists;
};

} // namespace phonio
