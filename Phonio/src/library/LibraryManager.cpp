#include "library/LibraryManager.h"

#include "database/DatabaseManager.h"

#include <algorithm>
#include <QDateTime>

namespace phonio {

LibraryManager::LibraryManager(DatabaseManager* db, QObject* parent)
    : QObject(parent)
    , m_db(db)
{
    reloadFromDatabase();
}

void LibraryManager::reloadFromDatabase()
{
    m_tracks = m_db->loadAllTracks();
    rebuildGroupings();
    emit libraryChanged();
}

std::optional<Track> LibraryManager::trackById(qint64 id) const
{
    for (const auto& track : m_tracks) {
        if (track.id == id)
            return track;
    }
    return std::nullopt;
}

Track LibraryManager::trackByRow(int row) const
{
    return (row >= 0 && row < m_tracks.size()) ? m_tracks.at(row) : Track{};
}

int LibraryManager::rowOfTrackId(qint64 id) const
{
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks.at(i).id == id)
            return i;
    }
    return -1;
}

QVector<Track> LibraryManager::tracksForIds(const QVector<qint64>& ids) const
{
    QVector<Track> result;
    result.reserve(ids.size());
    for (qint64 id : ids) {
        const auto track = trackById(id);
        if (track)
            result.append(*track);
    }
    return result;
}

void LibraryManager::rebuildGroupings()
{
    m_artists.clear();
    m_albums.clear();
    m_genres.clear();

    QHash<QString, Artist> artistMap;
    QHash<QString, Album> albumMap;
    QHash<QString, Genre> genreMap;

    for (const auto& track : m_tracks) {
        const QString artistKey = track.displayArtist().toLower();
        artistMap[artistKey].name = track.displayArtist();
        artistMap[artistKey].trackIds.append(track.id);

        const QString albumKey = (track.artist + QStringLiteral("\u001f") + track.album).toLower();
        if (!track.album.isEmpty()) {
            auto& album = albumMap[albumKey];
            album.name = track.album;
            album.artist = track.artist;
            album.year = track.year;
            album.trackIds.append(track.id);
        }

        const QString genreKey = track.genre.toLower();
        if (!track.genre.isEmpty()) {
            genreMap[genreKey].name = track.genre;
            genreMap[genreKey].trackIds.append(track.id);
        }
    }

    auto sortAlbum = [this](Album& a) {
        std::sort(a.trackIds.begin(), a.trackIds.end(), [this](qint64 lhs, qint64 rhs) {
            const auto l = trackById(lhs);
            const auto r = trackById(rhs);
            if (!l || !r)
                return lhs < rhs;
            if (l->discNumber != r->discNumber)
                return l->discNumber < r->discNumber;
            return l->trackNumber < r->trackNumber;
        });
    };

    m_artists = artistMap.values();
    m_albums = albumMap.values();
    m_genres = genreMap.values();
    for (auto& album : m_albums)
        sortAlbum(album);

    std::sort(m_artists.begin(), m_artists.end(),
              [](const Artist& a, const Artist& b) { return a.name.compare(b.name, Qt::CaseInsensitive) < 0; });
    std::sort(m_albums.begin(), m_albums.end(),
              [](const Album& a, const Album& b) { return a.name.compare(b.name, Qt::CaseInsensitive) < 0; });
    std::sort(m_genres.begin(), m_genres.end(),
              [](const Genre& a, const Genre& b) { return a.name.compare(b.name, Qt::CaseInsensitive) < 0; });
}

void LibraryManager::updateTrackInternal(const Track& updated)
{
    for (auto& track : m_tracks) {
        if (track.id == updated.id) {
            const bool groupingRelevant = track.artist != updated.artist
                || track.album != updated.album || track.genre != updated.genre;
            track = updated;
            m_db->updateTrackStats(track);
            if (groupingRelevant)
                rebuildGroupings();
            emit trackChanged(updated.id);
            return;
        }
    }
}

void LibraryManager::updateTrack(const Track& updated)
{
    updateTrackInternal(updated);
}

void LibraryManager::applyMetadataRefresh(const Track& updated)
{
    for (auto& track : m_tracks) {
        if (track.id == updated.id) {
            const QString filePath = track.filePath;
            const qint64 id = track.id;
            const int rating = track.rating;
            const int playCount = track.playCount;
            const qint64 lastPlayed = track.lastPlayedMs;
            const bool favorite = track.favorite;
            const QString lyricsPath = track.lyricsPath;
            track = updated;
            track.id = id;
            track.filePath = filePath;
            track.rating = rating;
            track.playCount = playCount;
            track.lastPlayedMs = lastPlayed;
            track.favorite = favorite;
            track.lyricsPath = lyricsPath;
            m_db->upsertTrack(track);
            rebuildGroupings();
            emit trackChanged(id);
            return;
        }
    }
}

void LibraryManager::incrementPlayCount(const Track& track)
{
    Track updated = track;
    ++updated.playCount;
    updated.lastPlayedMs = QDateTime::currentMSecsSinceEpoch();
    updateTrackInternal(updated);
}

void LibraryManager::setFavorite(const Track& track, bool favorite)
{
    Track updated = track;
    updated.favorite = favorite;
    updateTrackInternal(updated);
}

void LibraryManager::setRating(const Track& track, int rating)
{
    Track updated = track;
    updated.rating = qBound(0, rating, 5);
    updateTrackInternal(updated);
}

void LibraryManager::setLyricsPath(qint64 trackId, const QString& path)
{
    for (auto& track : m_tracks) {
        if (track.id == trackId) {
            track.lyricsPath = path;
            m_db->updateTrackLyricsPath(trackId, path);
            emit trackChanged(trackId);
            return;
        }
    }
}

void LibraryManager::removeTrack(qint64 trackId)
{
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks.at(i).id == trackId) {
            m_tracks.removeAt(i);
            m_db->removeTrack(trackId);
            rebuildGroupings();
            emit trackRemoved(trackId);
            emit libraryChanged();
            return;
        }
    }
}

} // namespace phonio
