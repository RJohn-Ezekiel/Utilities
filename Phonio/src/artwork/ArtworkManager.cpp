#include "artwork/ArtworkManager.h"

#include "metadata/MetadataManager.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QDateTime>
#include <QCryptographicHash>
#include <QDebug>

namespace phonio {

namespace {
constexpr int kMemoryCacheSize = 300;
}

ArtworkManager::ArtworkManager(MetadataManager* metadata, QObject* parent)
    : QObject(parent)
    , m_metadata(metadata)
    , m_memoryCache(kMemoryCacheSize)
{
}

QString ArtworkManager::cacheDir() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    return base + QStringLiteral("/artwork");
}

QString ArtworkManager::cacheKey(const QString& filePath) const
{
    QFileInfo info(filePath);
    const QString key = filePath + QStringLiteral("|") + QString::number(info.lastModified().toMSecsSinceEpoch());
    return QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex());
}

QString ArtworkManager::cacheFileForKey(const QString& key) const
{
    return cacheDir() + QLatin1Char('/') + key + QStringLiteral(".png");
}

QImage ArtworkManager::readImage(const QString& path)
{
    QImage image(path);
    if (!image.isNull())
        image.setDevicePixelRatio(1.0);
    return image;
}

QImage ArtworkManager::resolve(const QString& filePath) const
{
    // 1. embedded artwork
    if (m_metadata) {
        const auto result = m_metadata->read(filePath);
        if (result.ok && !result.artwork.isNull())
            return result.artwork;
    }
    // 2. folder.jpg / cover.jpg
    const QDir dir = QFileInfo(filePath).dir();
    const QString base = QFileInfo(filePath).completeBaseName();
    for (const QString& candidate : {base + QStringLiteral(".jpg"), base + QStringLiteral(".png")}) {
        const QString path = dir.filePath(candidate);
        if (QFileInfo::exists(path)) {
            const QImage image = readImage(path);
            if (!image.isNull())
                return image;
        }
    }
    for (const QString& candidate : {QStringLiteral("folder.jpg"), QStringLiteral("Folder.jpg"),
                                     QStringLiteral("FOLDER.JPG"), QStringLiteral("cover.jpg"),
                                     QStringLiteral("Cover.jpg"), QStringLiteral("COVER.JPG"),
                                     QStringLiteral("folder.png"), QStringLiteral("cover.png")}) {
        const QString path = dir.filePath(candidate);
        if (QFileInfo::exists(path)) {
            const QImage image = readImage(path);
            if (!image.isNull())
                return image;
        }
    }
    return {};
}

QImage ArtworkManager::makePlaceholder(int size) const
{
    QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient gradient(0, 0, size, size);
    gradient.setColorAt(0.0, QColor(58, 58, 58));
    gradient.setColorAt(1.0, QColor(38, 38, 38));
    p.setBrush(gradient);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(0, 0, size, size, size / 12.0, size / 12.0);
    p.setPen(QColor(255, 255, 255, 70));
    QFont font = p.font();
    font.setPixelSize(size * 0.55);
    font.setWeight(QFont::Light);
    p.setFont(font);
    const QString glyph = QStringLiteral("\u266B");
    const QRectF rect(0, 0, size, size);
    p.drawText(rect, Qt::AlignCenter, glyph);
    return image;
}

QPixmap ArtworkManager::artworkForPath(const QString& filePath)
{
    const QString key = cacheKey(filePath);
    if (QPixmap* cached = m_memoryCache.object(key))
        return *cached;

    QImage image = readImage(cacheFileForKey(key));
    if (image.isNull()) {
        image = resolve(filePath);
        if (image.isNull())
            image = makePlaceholder(512);
        QDir().mkpath(cacheDir());
        image.save(cacheFileForKey(key));
    }

    QPixmap pixmap = QPixmap::fromImage(image);
    m_memoryCache.insert(key, new QPixmap(pixmap));
    return pixmap;
}

QPixmap ArtworkManager::artworkFor(const Track& track)
{
    return artworkForPath(track.filePath);
}

QPixmap ArtworkManager::thumbnailFor(const Track& track, int size)
{
    const QPixmap art = artworkFor(track);
    if (art.isNull())
        return placeholder(size);
    QPixmap thumb = art.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    thumb = thumb.copy((thumb.width() - size) / 2, (thumb.height() - size) / 2, size, size);
    return thumb;
}

QPixmap ArtworkManager::placeholder(int size)
{
    return QPixmap::fromImage(makePlaceholder(size));
}

void ArtworkManager::invalidate(const Track& track)
{
    const QString key = cacheKey(track.filePath);
    m_memoryCache.remove(key);
    QFile::remove(cacheFileForKey(key));
}

} // namespace phonio
