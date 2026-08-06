#pragma once

#include "core/Types.h"

#include <QObject>
#include <QImage>
#include <optional>

namespace phonio {

struct AudioInfo
{
    int durationMs = 0;
    int bitrate = 0;
    int sampleRate = 0;
    QString fileType;
};

// TagLib facade: reads/writes metadata and embedded artwork for
// MP3, FLAC, MP4 (M4A) and Ogg Vorbis/Opus files.
class MetadataManager : public QObject
{
    Q_OBJECT

public:
    struct ReadResult
    {
        bool ok = false;
        QString error;
        Track track;        // includes audio properties, no stats
        QImage artwork;
    };

    explicit MetadataManager(QObject* parent = nullptr);

    ReadResult read(const QString& filePath) const;

    // Returns false when the format does not support artwork or on failure.
    bool writeArtwork(const QString& filePath, const QImage& image, QString* error = nullptr) const;
    bool removeArtwork(const QString& filePath, QString* error = nullptr) const;
    bool artworkSupported(const QString& filePath) const;

    // Updates tag fields from `track` (title/artist/...). Play stats are ignored.
    bool writeTags(const QString& filePath, const Track& track, QString* error = nullptr) const;

    bool isSupportedFile(const QString& filePath) const;

private:
    static bool writeArtworkMp3(const QString& filePath, const QImage& image, bool remove, QString* error);
    static bool writeArtworkFlac(const QString& filePath, const QImage& image, bool remove, QString* error);
    static bool writeArtworkMp4(const QString& filePath, const QImage& image, bool remove, QString* error);

    Q_DISABLE_COPY(MetadataManager)
};

} // namespace phonio
