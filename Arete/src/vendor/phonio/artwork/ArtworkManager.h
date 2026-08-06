#pragma once

#include "core/Types.h"

#include <QObject>
#include <QPixmap>
#include <QCache>
#include <QMutex>

class QPixmap;

namespace phonio {

class MetadataManager;

// Resolves album art with priority: embedded cover -> folder.jpg -> cover.jpg ->
// generated placeholder. Caches thumbnails in memory and full art on disk.
class ArtworkManager : public QObject
{
    Q_OBJECT

public:
    explicit ArtworkManager(MetadataManager* metadata, QObject* parent = nullptr);

    // Full-resolution artwork for a track. Never returns a null pixmap.
    QPixmap artworkFor(const Track& track);
    QPixmap artworkForPath(const QString& filePath);

    // Scaled, cached thumbnail (thread-safe on main thread only).
    QPixmap thumbnailFor(const Track& track, int size);
    QPixmap placeholder(int size);

    void invalidate(const Track& track);

    static QImage readImage(const QString& path);

private:
    QImage resolve(const QString& filePath) const;
    QImage readFolderImage(const QDir& dir) const;
    QImage makePlaceholder(int size) const;
    QString cacheKey(const QString& filePath) const;
    QString cacheFileForKey(const QString& key) const;
    QString cacheDir() const;

    MetadataManager* m_metadata;
    QCache<QString, QPixmap> m_memoryCache;
    QMutex m_mutex;
};

} // namespace phonio
