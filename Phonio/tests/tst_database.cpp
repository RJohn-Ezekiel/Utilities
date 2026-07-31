#include <QtTest>

#include "database/DatabaseManager.h"

#include <QTemporaryDir>

using namespace phonio;

class TestDatabase : public QObject
{
    Q_OBJECT

private slots:
    void trackRoundTrip();
    void upsertUpdatesExisting();
    void playlistCrud();
    void queuePersistence();
    void statsPersistence();
};

void TestDatabase::trackRoundTrip()
{
    QTemporaryDir dir;
    DatabaseManager db(dir.filePath(QStringLiteral("test.db")));

    Track track;
    track.filePath = QStringLiteral("/music/hello.mp3");
    track.title = QStringLiteral("Hello");
    track.artist = QStringLiteral("Artist");
    track.album = QStringLiteral("Album");
    track.genre = QStringLiteral("Rock");
    track.year = 2020;
    track.trackNumber = 3;
    track.discNumber = 1;
    track.composer = QStringLiteral("Composer");
    track.comment = QStringLiteral("Comment");
    track.durationMs = 240000;
    track.bitrate = 320;
    track.sampleRate = 44100;
    track.fileType = QStringLiteral("mp3");

    const qint64 id = db.upsertTrack(track);
    QVERIFY(id > 0);

    const auto loaded = db.trackByPath(QStringLiteral("/music/hello.mp3"));
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->id, id);
    QCOMPARE(loaded->title, QStringLiteral("Hello"));
    QCOMPARE(loaded->durationMs, 240000);
    QCOMPARE(loaded->bitrate, 320);

    const auto byId = db.trackById(id);
    QVERIFY(byId.has_value());
    QCOMPARE(byId->filePath, QStringLiteral("/music/hello.mp3"));
}

void TestDatabase::upsertUpdatesExisting()
{
    QTemporaryDir dir;
    DatabaseManager db(dir.filePath(QStringLiteral("test.db")));

    Track track;
    track.filePath = QStringLiteral("/music/a.mp3");
    track.title = QStringLiteral("Old");
    track.artist = QStringLiteral("Old Artist");
    const qint64 id = db.upsertTrack(track);

    Track updated;
    updated.filePath = QStringLiteral("/music/a.mp3");
    updated.title = QStringLiteral("New");
    updated.artist = QStringLiteral("New Artist");
    const qint64 newId = db.upsertTrack(updated);

    QCOMPARE(newId, id); // same row
    const auto loaded = db.trackByPath(QStringLiteral("/music/a.mp3"));
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->title, QStringLiteral("New"));
    QCOMPARE(loaded->artist, QStringLiteral("New Artist"));
}

void TestDatabase::playlistCrud()
{
    QTemporaryDir dir;
    DatabaseManager db(dir.filePath(QStringLiteral("test.db")));

    Track a, b;
    a.filePath = QStringLiteral("/music/a.mp3");
    b.filePath = QStringLiteral("/music/b.mp3");
    const qint64 aId = db.upsertTrack(a);
    const qint64 bId = db.upsertTrack(b);

    const qint64 playlistId = db.createPlaylist(QStringLiteral("Road Trip"));
    QVERIFY(playlistId > 0);
    QCOMPARE(db.loadPlaylists().size(), 1);

    QVERIFY(db.renamePlaylist(playlistId, QStringLiteral("Road Trip v2")));
    QCOMPARE(db.loadPlaylists().at(0).name, QStringLiteral("Road Trip v2"));

    db.setPlaylistTracks(playlistId, {aId, bId});
    QCOMPARE(db.playlistTrackIds(playlistId), QVector<qint64>({aId, bId}));

    db.setPlaylistTracks(playlistId, {bId});
    QCOMPARE(db.playlistTrackIds(playlistId), QVector<qint64>({bId}));

    db.deletePlaylist(playlistId);
    QVERIFY(db.loadPlaylists().isEmpty());
    QVERIFY(db.playlistTrackIds(playlistId).isEmpty());
}

void TestDatabase::queuePersistence()
{
    QTemporaryDir dir;
    DatabaseManager db(dir.filePath(QStringLiteral("test.db")));

    Track a, b, c;
    a.filePath = QStringLiteral("/music/a.mp3");
    b.filePath = QStringLiteral("/music/b.mp3");
    c.filePath = QStringLiteral("/music/c.mp3");
    const qint64 aId = db.upsertTrack(a);
    const qint64 bId = db.upsertTrack(b);
    const qint64 cId = db.upsertTrack(c);

    db.saveQueue({aId, bId, cId});
    QCOMPARE(db.loadQueue(), QVector<qint64>({aId, bId, cId}));

    db.saveQueue({cId});
    QCOMPARE(db.loadQueue(), QVector<qint64>({cId}));

    db.saveQueue({});
    QVERIFY(db.loadQueue().isEmpty());
}

void TestDatabase::statsPersistence()
{
    QTemporaryDir dir;
    DatabaseManager db(dir.filePath(QStringLiteral("test.db")));

    Track track;
    track.filePath = QStringLiteral("/music/a.mp3");
    const qint64 id = db.upsertTrack(track);
    QVERIFY(id > 0);

    Track withStats;
    withStats.id = id;
    withStats.filePath = track.filePath;
    withStats.rating = 4;
    withStats.playCount = 17;
    withStats.favorite = true;
    withStats.lastPlayedMs = 1234567;
    withStats.lyricsPath = QStringLiteral("/lyrics/a.lrc");
    db.updateTrackStats(withStats);
    db.updateTrackLyricsPath(id, QStringLiteral("/lyrics/a.lrc"));

    const auto loaded = db.trackById(id);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->rating, 4);
    QCOMPARE(loaded->playCount, 17);
    QCOMPARE(loaded->favorite, true);
    QCOMPARE(loaded->lastPlayedMs, 1234567);
    QCOMPARE(loaded->lyricsPath, QStringLiteral("/lyrics/a.lrc"));
}

QTEST_MAIN(TestDatabase)
#include "tst_database.moc"
