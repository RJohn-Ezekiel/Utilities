#include "settings/SettingsManager.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>

namespace phonio {

namespace {
constexpr const char* kMusicFolders = "library/musicFolders";
constexpr const char* kVolume = "playback/volume";
constexpr const char* kRememberPosition = "playback/rememberPosition";
constexpr const char* kShuffle = "playback/shuffle";
constexpr const char* kRepeat = "playback/repeatMode";
constexpr const char* kAutoLyrics = "lyrics/autoLoad";
constexpr const char* kAccent = "ui/accentColor";
}

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
    , m_settings(new QSettings(QSettings::IniFormat, QSettings::UserScope,
                               QStringLiteral("Phonio"), QStringLiteral("Phonio"), this))
{
    if (!m_settings->contains(QLatin1String(kVolume)))
        setVolume(0.9);
    if (!m_settings->contains(QLatin1String(kAutoLyrics)))
        setAutoLoadLyrics(true);
    if (!m_settings->contains(QLatin1String(kRememberPosition)))
        setRememberPosition(true);
    if (!m_settings->contains(QLatin1String(kRepeat)))
        setRepeatMode(0);
    if (!m_settings->contains(QLatin1String(kAccent)))
        setAccentColor(QColor(216, 216, 216));
}

void SettingsManager::sync()
{
    m_settings->sync();
}

QStringList SettingsManager::musicFolders() const
{
    auto folders = m_settings->value(QLatin1String(kMusicFolders)).toStringList();
    if (folders.isEmpty()) {
        const QString music = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        if (!music.isEmpty() && QDir(music).exists())
            folders << music;
    }
    return folders;
}

void SettingsManager::addMusicFolder(const QString& path)
{
    auto folders = musicFolders();
    const QString canonical = QDir::cleanPath(path);
    if (!folders.contains(canonical))
        folders << canonical;
    m_settings->setValue(QLatin1String(kMusicFolders), folders);
}

void SettingsManager::removeMusicFolder(const QString& path)
{
    auto folders = musicFolders();
    folders.removeAll(QDir::cleanPath(path));
    m_settings->setValue(QLatin1String(kMusicFolders), folders);
}

double SettingsManager::volume() const
{
    return m_settings->value(QLatin1String(kVolume), 0.9).toDouble();
}

void SettingsManager::setVolume(double volume)
{
    m_settings->setValue(QLatin1String(kVolume), qBound(0.0, volume, 1.0));
}

bool SettingsManager::rememberPosition() const
{
    return m_settings->value(QLatin1String(kRememberPosition), true).toBool();
}

void SettingsManager::setRememberPosition(bool enabled)
{
    m_settings->setValue(QLatin1String(kRememberPosition), enabled);
}

qint64 SettingsManager::savedPositionForTrack(qint64 trackId) const
{
    return m_settings->value(QStringLiteral("playback/position/%1").arg(trackId), 0).toLongLong();
}

void SettingsManager::savePositionForTrack(qint64 trackId, qint64 positionMs)
{
    m_settings->setValue(QStringLiteral("playback/position/%1").arg(trackId), positionMs);
}

bool SettingsManager::isShuffleEnabled() const
{
    return m_settings->value(QLatin1String(kShuffle), false).toBool();
}

void SettingsManager::setShuffleEnabled(bool enabled)
{
    m_settings->setValue(QLatin1String(kShuffle), enabled);
}

int SettingsManager::repeatMode() const
{
    return m_settings->value(QLatin1String(kRepeat), 0).toInt();
}

void SettingsManager::setRepeatMode(int mode)
{
    m_settings->setValue(QLatin1String(kRepeat), qBound(0, mode, 2));
}

bool SettingsManager::autoLoadLyrics() const
{
    return m_settings->value(QLatin1String(kAutoLyrics), true).toBool();
}

void SettingsManager::setAutoLoadLyrics(bool enabled)
{
    m_settings->setValue(QLatin1String(kAutoLyrics), enabled);
}

QColor SettingsManager::accentColor() const
{
    const QString stored = m_settings->value(QLatin1String(kAccent), QColor(216, 216, 216).name()).toString();
    if (stored == QLatin1String("#1ed760"))
        return QColor(216, 216, 216);
    return QColor(stored);
}

void SettingsManager::setAccentColor(const QColor& color)
{
    m_settings->setValue(QLatin1String(kAccent), color.name(QColor::HexRgb));
}

QVariant SettingsManager::value(const QString& key, const QVariant& fallback) const
{
    return m_settings->value(key, fallback);
}

void SettingsManager::setValue(const QString& key, const QVariant& value)
{
    m_settings->setValue(key, value);
}

} // namespace phonio
