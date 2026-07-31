#pragma once

#include <QString>
#include <QVector>
#include <QMetaType>
#include <QFileInfo>

namespace phonio {

struct Track
{
    qint64 id = -1;             // database row id, -1 when not persisted
    QString filePath;
    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString genre;
    QString composer;
    QString comment;
    int year = 0;
    int trackNumber = 0;
    int discNumber = 0;
    int durationMs = 0;
    int bitrate = 0;
    int sampleRate = 0;
    QString fileType;           // "mp3", "flac", ...
    int rating = 0;             // 0..5
    int playCount = 0;
    qint64 lastPlayedMs = 0;    // unix epoch ms
    bool favorite = false;
    QString lyricsPath;         // attached .lrc (empty -> auto-detect beside file)

    bool isValid() const { return !filePath.isEmpty(); }
    QString displayTitle() const { return title.isEmpty() ? QFileInfo(filePath).completeBaseName() : title; }
    QString displayArtist() const { return artist.isEmpty() ? QStringLiteral("Unknown Artist") : artist; }
    QString displayAlbum() const { return album.isEmpty() ? QStringLiteral("Unknown Album") : album; }
    QString displayGenre() const { return genre.isEmpty() ? QStringLiteral("Unknown Genre") : genre; }
    bool operator==(const Track& o) const { return id == o.id && filePath == o.filePath; }
};

struct Album
{
    QString name;
    QString artist;
    QVector<qint64> trackIds;   // sorted by disc/track number
    int year = 0;
};

struct Artist
{
    QString name;
    QVector<qint64> trackIds;
};

struct Genre
{
    QString name;
    QVector<qint64> trackIds;
};

struct PlaylistInfo
{
    qint64 id = -1;
    QString name;
    int trackCount = 0;
};

struct LyricLine
{
    qint64 timeMs = 0;
    QString text;
};

enum class RepeatMode { Off, All, One };

QString formatDurationMs(qint64 ms);
QString formatTime(qint64 ms);          // m:ss or h:mm:ss
QString formatBitrate(int kbps);

} // namespace phonio

Q_DECLARE_METATYPE(phonio::Track)
Q_DECLARE_METATYPE(QVector<phonio::LyricLine>)
Q_DECLARE_METATYPE(phonio::RepeatMode)
