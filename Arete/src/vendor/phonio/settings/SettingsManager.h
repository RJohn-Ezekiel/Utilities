#pragma once

#include <QObject>
#include <QStringList>
#include <QColor>
#include <QVariant>

class QSettings;

namespace phonio {

// Preferences. Single source of truth for user settings; persisted with QSettings.
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingsManager(QObject* parent = nullptr);

    void sync();

    // --- Library -------------------------------------------------------
    QStringList musicFolders() const;
    void addMusicFolder(const QString& path);
    void removeMusicFolder(const QString& path);

    // --- Playback ------------------------------------------------------
    double volume() const;                  // 0.0 .. 1.0
    void setVolume(double volume);

    bool rememberPosition() const;
    void setRememberPosition(bool enabled);

    qint64 savedPositionForTrack(qint64 trackId) const;
    void savePositionForTrack(qint64 trackId, qint64 positionMs);

    bool isShuffleEnabled() const;
    void setShuffleEnabled(bool enabled);
    int repeatMode() const;                 // 0=Off 1=All 2=One
    void setRepeatMode(int mode);

    bool autoLoadLyrics() const;
    void setAutoLoadLyrics(bool enabled);

    // --- Appearance ----------------------------------------------------
    QColor accentColor() const;
    void setAccentColor(const QColor& color);

    // Generic access (used by future features / plugins)
    QVariant value(const QString& key, const QVariant& fallback = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);

private:
    QSettings* m_settings;
};

} // namespace phonio
