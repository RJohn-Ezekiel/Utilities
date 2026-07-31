#include "database/DatabaseManager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QVariant>
#include <QDebug>
#include <QThread>
#include <QThreadStorage>
#include <QDateTime>
#include <QFileInfo>

#include <cstring>
#include <atomic>

namespace phonio {

namespace {
constexpr int kSchemaVersion = 1;

std::atomic<int> g_connectionCounter{0};

Track trackFromQuery(const QSqlQuery& query)
{
    Track t;
    t.id = query.value(0).toLongLong();
    t.filePath = query.value(1).toString();
    t.title = query.value(2).toString();
    t.artist = query.value(3).toString();
    t.album = query.value(4).toString();
    t.albumArtist = query.value(5).toString();
    t.genre = query.value(6).toString();
    t.year = query.value(7).toInt();
    t.trackNumber = query.value(8).toInt();
    t.discNumber = query.value(9).toInt();
    t.composer = query.value(10).toString();
    t.comment = query.value(11).toString();
    t.durationMs = query.value(12).toInt();
    t.bitrate = query.value(13).toInt();
    t.sampleRate = query.value(14).toInt();
    t.fileType = query.value(15).toString();
    t.rating = query.value(16).toInt();
    t.playCount = query.value(17).toInt();
    t.lastPlayedMs = query.value(18).toLongLong();
    t.favorite = query.value(19).toBool();
    t.lyricsPath = query.value(20).toString();
    return t;
}

constexpr const char* kTrackColumns =
    "id, file_path, title, artist, album, album_artist, genre, year, "
    "track_number, disc_number, composer, comment, duration_ms, bitrate, "
    "sample_rate, file_type, rating, play_count, last_played_ms, favorite, lyrics_path";
}

DatabaseManager::DatabaseManager(const QString& databasePath, QObject* parent)
    : QObject(parent)
    , m_databasePath(databasePath)
    , m_mainConnection(QStringLiteral("phonio_main_%1").arg(g_connectionCounter.fetch_add(1)))
{
    QDir().mkpath(QFileInfo(databasePath).absolutePath());
    open();
}

DatabaseManager::~DatabaseManager()
{
    {
        QSqlDatabase db = QSqlDatabase::database(m_mainConnection, false);
        if (db.isValid())
            db.close();
    }
    QSqlDatabase::removeDatabase(m_mainConnection);
}

void DatabaseManager::open()
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_mainConnection);
    db.setDatabaseName(m_databasePath);
    db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        return;
    }
    db.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    db.exec(QStringLiteral("PRAGMA foreign_keys=ON"));

    QSqlQuery query(db);
    query.exec(QStringLiteral("PRAGMA user_version"));
    int version = 0;
    if (query.next())
        version = query.value(0).toInt();
    migrate(version);
}

void DatabaseManager::migrate(int fromVersion)
{
    if (fromVersion < 1) {
        QSqlQuery query(QSqlDatabase::database(m_mainConnection));
        const QStringList statements = {
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS tracks ("
                " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                " file_path TEXT NOT NULL UNIQUE,"
                " title TEXT, artist TEXT, album TEXT, album_artist TEXT,"
                " genre TEXT, year INTEGER DEFAULT 0,"
                " track_number INTEGER DEFAULT 0, disc_number INTEGER DEFAULT 0,"
                " composer TEXT, comment TEXT,"
                " duration_ms INTEGER DEFAULT 0, bitrate INTEGER DEFAULT 0,"
                " sample_rate INTEGER DEFAULT 0, file_type TEXT,"
                " rating INTEGER DEFAULT 0, play_count INTEGER DEFAULT 0,"
                " last_played_ms INTEGER DEFAULT 0, favorite INTEGER DEFAULT 0,"
                " lyrics_path TEXT DEFAULT ''"
                ")"),
            QStringLiteral("CREATE INDEX IF NOT EXISTS idx_tracks_path ON tracks(file_path)"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS playlists ("
                " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                " name TEXT NOT NULL,"
                " created_ms INTEGER NOT NULL"
                ")"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS playlist_tracks ("
                " playlist_id INTEGER NOT NULL REFERENCES playlists(id) ON DELETE CASCADE,"
                " position INTEGER NOT NULL,"
                " track_id INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE,"
                " PRIMARY KEY (playlist_id, position)"
                ")"),
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS queue ("
                " position INTEGER PRIMARY KEY,"
                " track_id INTEGER NOT NULL REFERENCES tracks(id) ON DELETE CASCADE"
                ")"),
        };
        for (const auto& sql : statements) {
            if (!query.exec(sql))
                qWarning() << "Migration failed:" << query.lastError().text() << sql;
        }
        QSqlQuery versionQuery(QSqlDatabase::database(m_mainConnection));
        versionQuery.exec(QStringLiteral("PRAGMA user_version=1"));
    }
}

// ---------------------------------------------------------------------------
// Library
// ---------------------------------------------------------------------------

QVector<Track> DatabaseManager::loadAllTracks() const
{
    QVector<Track> result;
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral("SELECT %1 FROM tracks ORDER BY title COLLATE NOCASE").arg(QLatin1String(kTrackColumns)));
    if (!query.exec()) {
        qWarning() << "loadAllTracks failed:" << query.lastError().text();
        return result;
    }
    while (query.next())
        result.append(trackFromQuery(query));
    return result;
}

std::optional<Track> DatabaseManager::trackById(qint64 id) const
{
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral("SELECT %1 FROM tracks WHERE id = ?").arg(QLatin1String(kTrackColumns)));
    query.addBindValue(id);
    if (!query.exec() || !query.next())
        return std::nullopt;
    return trackFromQuery(query);
}

std::optional<Track> DatabaseManager::trackByPath(const QString& filePath) const
{
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral("SELECT %1 FROM tracks WHERE file_path = ?").arg(QLatin1String(kTrackColumns)));
    query.addBindValue(filePath);
    if (!query.exec() || !query.next())
        return std::nullopt;
    return trackFromQuery(query);
}

qint64 DatabaseManager::upsertTrack(const Track& track)
{
    // Uses a dedicated connection so worker threads don't share the main one.
    // The cache is keyed per thread but must be revalidated against the
    // current database file (tests / multiple instances may differ).
    static QThreadStorage<QString> connectionKey;
    QString key;
    bool stale = false;
    if (connectionKey.hasLocalData()) {
        key = connectionKey.localData();
        {
            QSqlDatabase cached = QSqlDatabase::database(key, false);
            if (!cached.isValid() || cached.databaseName() != m_databasePath) {
                if (cached.isValid())
                    cached.close();
                stale = true;
            }
        }
        if (stale) {
            QSqlDatabase::removeDatabase(key);
            key.clear();
        }
    }
    if (key.isEmpty()) {
        key = QStringLiteral("phonio_worker_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), key);
        db.setDatabaseName(m_databasePath);
        db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
        db.open();
        connectionKey.setLocalData(key);
    }

    QSqlDatabase db = QSqlDatabase::database(key);
    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO tracks (file_path, title, artist, album, album_artist, genre, year,"
        " track_number, disc_number, composer, comment, duration_ms, bitrate, sample_rate,"
        " file_type, lyrics_path)"
        " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        " ON CONFLICT(file_path) DO UPDATE SET"
        " title=excluded.title, artist=excluded.artist, album=excluded.album,"
        " album_artist=excluded.album_artist, genre=excluded.genre, year=excluded.year,"
        " track_number=excluded.track_number, disc_number=excluded.disc_number,"
        " composer=excluded.composer, comment=excluded.comment,"
        " duration_ms=excluded.duration_ms, bitrate=excluded.bitrate,"
        " sample_rate=excluded.sample_rate, file_type=excluded.file_type"
        " RETURNING id"));
    query.addBindValue(track.filePath);
    query.addBindValue(track.title);
    query.addBindValue(track.artist);
    query.addBindValue(track.album);
    query.addBindValue(track.albumArtist);
    query.addBindValue(track.genre);
    query.addBindValue(track.year);
    query.addBindValue(track.trackNumber);
    query.addBindValue(track.discNumber);
    query.addBindValue(track.composer);
    query.addBindValue(track.comment);
    query.addBindValue(track.durationMs);
    query.addBindValue(track.bitrate);
    query.addBindValue(track.sampleRate);
    query.addBindValue(track.fileType);
    query.addBindValue(track.lyricsPath);
    if (!query.exec() || !query.next()) {
        qWarning() << "upsertTrack failed:" << query.lastError().text() << track.filePath;
        return -1;
    }
    return query.value(0).toLongLong();
}

void DatabaseManager::updateTrackStats(const Track& track)
{
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral(
        "UPDATE tracks SET rating=?, play_count=?, last_played_ms=?, favorite=?"
        " WHERE id=?"));
    query.addBindValue(track.rating);
    query.addBindValue(track.playCount);
    query.addBindValue(track.lastPlayedMs);
    query.addBindValue(track.favorite ? 1 : 0);
    query.addBindValue(track.id);
    if (!query.exec())
        qWarning() << "updateTrackStats failed:" << query.lastError().text();
}

void DatabaseManager::updateTrackLyricsPath(qint64 trackId, const QString& lyricsPath)
{
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral("UPDATE tracks SET lyrics_path=? WHERE id=?"));
    query.addBindValue(lyricsPath);
    query.addBindValue(trackId);
    if (!query.exec())
        qWarning() << "updateTrackLyricsPath failed:" << query.lastError().text();
}

void DatabaseManager::removeTracksNotIn(const QSet<QString>& existingPaths)
{
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral("SELECT id, file_path FROM tracks"));
    if (!query.exec())
        return;
    QVector<qint64> stale;
    while (query.next()) {
        if (!existingPaths.contains(query.value(1).toString()))
            stale.append(query.value(0).toLongLong());
    }
    QSqlQuery del(QSqlDatabase::database(m_mainConnection));
    del.prepare(QStringLiteral("DELETE FROM tracks WHERE id=?"));
    for (qint64 id : stale) {
        del.addBindValue(id);
        if (!del.exec())
            qWarning() << "removeTracksNotIn failed:" << del.lastError().text();
    }
}

void DatabaseManager::removeTrack(qint64 trackId)
{
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral("DELETE FROM tracks WHERE id=?"));
    query.addBindValue(trackId);
    if (!query.exec())
        qWarning() << "removeTrack failed:" << query.lastError().text();
}

// ---------------------------------------------------------------------------
// Playlists
// ---------------------------------------------------------------------------

QVector<PlaylistInfo> DatabaseManager::loadPlaylists() const
{
    QVector<PlaylistInfo> result;
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral(
        "SELECT p.id, p.name, COUNT(pt.position) FROM playlists p"
        " LEFT JOIN playlist_tracks pt ON pt.playlist_id = p.id"
        " GROUP BY p.id ORDER BY p.name COLLATE NOCASE"));
    if (!query.exec()) {
        qWarning() << "loadPlaylists failed:" << query.lastError().text();
        return result;
    }
    while (query.next()) {
        PlaylistInfo info;
        info.id = query.value(0).toLongLong();
        info.name = query.value(1).toString();
        info.trackCount = query.value(2).toInt();
        result.append(info);
    }
    return result;
}

qint64 DatabaseManager::createPlaylist(const QString& name)
{
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral("INSERT INTO playlists (name, created_ms) VALUES (?, ?)"));
    query.addBindValue(name);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    if (!query.exec()) {
        qWarning() << "createPlaylist failed:" << query.lastError().text();
        return -1;
    }
    return query.lastInsertId().toLongLong();
}

bool DatabaseManager::renamePlaylist(qint64 playlistId, const QString& newName)
{
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral("UPDATE playlists SET name=? WHERE id=?"));
    query.addBindValue(newName);
    query.addBindValue(playlistId);
    return query.exec();
}

void DatabaseManager::deletePlaylist(qint64 playlistId)
{
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral("DELETE FROM playlists WHERE id=?"));
    query.addBindValue(playlistId);
    if (!query.exec())
        qWarning() << "deletePlaylist failed:" << query.lastError().text();
}

QVector<qint64> DatabaseManager::playlistTrackIds(qint64 playlistId) const
{
    QVector<qint64> result;
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral(
        "SELECT track_id FROM playlist_tracks WHERE playlist_id=? ORDER BY position"));
    query.addBindValue(playlistId);
    if (!query.exec()) {
        qWarning() << "playlistTrackIds failed:" << query.lastError().text();
        return result;
    }
    while (query.next())
        result.append(query.value(0).toLongLong());
    return result;
}

void DatabaseManager::setPlaylistTracks(qint64 playlistId, const QVector<qint64>& trackIds)
{
    QSqlDatabase db = QSqlDatabase::database(m_mainConnection);
    QSqlQuery query(db);
    if (!db.transaction()) {
        qWarning() << "setPlaylistTracks: cannot start transaction";
    }
    query.prepare(QStringLiteral("DELETE FROM playlist_tracks WHERE playlist_id=?"));
    query.addBindValue(playlistId);
    query.exec();
    QSqlQuery insert(db);
    insert.prepare(QStringLiteral("INSERT INTO playlist_tracks (playlist_id, position, track_id) VALUES (?,?,?)"));
    for (int i = 0; i < trackIds.size(); ++i) {
        insert.addBindValue(playlistId);
        insert.addBindValue(i);
        insert.addBindValue(trackIds.at(i));
        if (!insert.exec())
            qWarning() << "setPlaylistTracks insert failed:" << insert.lastError().text();
    }
    db.commit();
}

// ---------------------------------------------------------------------------
// Queue
// ---------------------------------------------------------------------------

QVector<qint64> DatabaseManager::loadQueue() const
{
    QVector<qint64> result;
    QSqlQuery query(QSqlDatabase::database(m_mainConnection));
    query.prepare(QStringLiteral("SELECT track_id FROM queue ORDER BY position"));
    if (!query.exec()) {
        qWarning() << "loadQueue failed:" << query.lastError().text();
        return result;
    }
    while (query.next())
        result.append(query.value(0).toLongLong());
    return result;
}

void DatabaseManager::saveQueue(const QVector<qint64>& trackIds)
{
    QSqlDatabase db = QSqlDatabase::database(m_mainConnection);
    QSqlQuery query(db);
    db.transaction();
    query.exec(QStringLiteral("DELETE FROM queue"));
    QSqlQuery insert(db);
    insert.prepare(QStringLiteral("INSERT INTO queue (position, track_id) VALUES (?,?)"));
    for (int i = 0; i < trackIds.size(); ++i) {
        insert.addBindValue(i);
        insert.addBindValue(trackIds.at(i));
        if (!insert.exec())
            qWarning() << "saveQueue insert failed:" << insert.lastError().text();
    }
    db.commit();
}

} // namespace phonio
