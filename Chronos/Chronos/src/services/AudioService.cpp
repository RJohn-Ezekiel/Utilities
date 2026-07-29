#include "AudioService.h"

#include <QSoundEffect>
#include <QUrl>
#include <QFile>
#include <QDebug>

namespace chronos {

AudioService::AudioService(QObject* parent)
    : QObject(parent)
{
    loadSound();
}

AudioService::~AudioService() = default;

void AudioService::playNotification()
{
    if (!m_enabled) {
        return;
    }

    if (m_effect && m_effect->isLoaded()) {
        m_effect->play();
    }
}

void AudioService::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool AudioService::isEnabled() const
{
    return m_enabled;
}

void AudioService::loadSound()
{
    const QStringList searchPaths = {
        QStringLiteral(":/sounds/notification.wav"),
        QStringLiteral("/usr/share/chronos/sounds/notification.wav"),
        QStringLiteral("/usr/local/share/chronos/sounds/notification.wav")
    };

    for (const auto& path : searchPaths) {
        if (QFile::exists(path)) {
            m_effect = new QSoundEffect(this);
            m_effect->setSource(QUrl::fromLocalFile(path));
            m_effect->setVolume(0.5);
            return;
        }
    }

    qDebug() << "AudioService: notification sound not found, notifications will be silent";
}

} // namespace chronos
