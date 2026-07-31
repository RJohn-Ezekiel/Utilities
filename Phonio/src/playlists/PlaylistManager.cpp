#include "playlists/PlaylistManager.h"

#include "database/DatabaseManager.h"
#include "library/LibraryManager.h"

#include <QSet>

namespace phonio {

PlaylistManager::PlaylistManager(DatabaseManager* db, LibraryManager* library, QObject* parent)
    : QObject(parent)
    , m_db(db)
    , m_library(library)
{
    reload();
}

void PlaylistManager::reload()
{
    m_playlists = m_db->loadPlaylists();
    emit playlistsChanged();
}

std::optional<PlaylistInfo> PlaylistManager::playlistById(qint64 id) const
{
    for (const auto& playlist : m_playlists) {
        if (playlist.id == id)
            return playlist;
    }
    return std::nullopt;
}

qint64 PlaylistManager::createPlaylist(const QString& name)
{
    const qint64 id = m_db->createPlaylist(name);
    if (id >= 0)
        reload();
    return id;
}

bool PlaylistManager::renamePlaylist(qint64 playlistId, const QString& newName)
{
    const bool ok = m_db->renamePlaylist(playlistId, newName);
    if (ok)
        reload();
    return ok;
}

void PlaylistManager::deletePlaylist(qint64 playlistId)
{
    m_db->deletePlaylist(playlistId);
    reload();
}

QVector<qint64> PlaylistManager::trackIdsOf(qint64 playlistId) const
{
    return m_db->playlistTrackIds(playlistId);
}

QVector<Track> PlaylistManager::tracksOf(qint64 playlistId) const
{
    return m_library->tracksForIds(m_db->playlistTrackIds(playlistId));
}

void PlaylistManager::setTracks(qint64 playlistId, const QVector<qint64>& trackIds)
{
    m_db->setPlaylistTracks(playlistId, trackIds);
    reload();
    emit playlistContentChanged(playlistId);
}

void PlaylistManager::appendTracks(qint64 playlistId, const QVector<qint64>& trackIds)
{
    auto ids = m_db->playlistTrackIds(playlistId);
    QSet<qint64> existing(ids.cbegin(), ids.cend());
    for (qint64 id : trackIds) {
        if (!existing.contains(id)) {
            ids.append(id);
            existing.insert(id);
        }
    }
    setTracks(playlistId, ids);
}

void PlaylistManager::removeTrackFromPlaylist(qint64 playlistId, qint64 trackId)
{
    auto ids = m_db->playlistTrackIds(playlistId);
    ids.removeAll(trackId);
    setTracks(playlistId, ids);
}

} // namespace phonio
