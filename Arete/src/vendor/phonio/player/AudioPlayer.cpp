#include "player/AudioPlayer.h"

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QFileInfo>

namespace phonio {

AudioPlayer::AudioPlayer(QObject* parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_output(new QAudioOutput(this))
{
    m_player->setAudioOutput(m_output);

    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        State mapped = State::Stopped;
        switch (state) {
        case QMediaPlayer::PlayingState: mapped = State::Playing; break;
        case QMediaPlayer::PausedState: mapped = State::Paused; break;
        case QMediaPlayer::StoppedState: mapped = State::Stopped; break;
        }
        emit stateChanged(mapped);
    });
    connect(m_player, &QMediaPlayer::positionChanged, this, &AudioPlayer::positionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &AudioPlayer::durationChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::EndOfMedia)
                    emit mediaEnded();
            });
    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString& errorString) {
                emit playbackFailed(errorString);
            });
    connect(m_output, &QAudioOutput::volumeChanged, this, &AudioPlayer::volumeChanged);
}

bool AudioPlayer::playFile(const QString& filePath)
{
    m_player->setSource(QUrl::fromLocalFile(filePath));
    m_player->play();
    return true;
}

void AudioPlayer::play()
{
    m_player->play();
}

void AudioPlayer::pause()
{
    m_player->pause();
}

void AudioPlayer::toggle()
{
    if (state() == State::Playing)
        pause();
    else
        play();
}

void AudioPlayer::stop()
{
    m_player->stop();
}

void AudioPlayer::seek(qint64 positionMs)
{
    m_player->setPosition(qMax<qint64>(0, positionMs));
}

qint64 AudioPlayer::positionMs() const
{
    return m_player->position();
}

qint64 AudioPlayer::durationMs() const
{
    return m_player->duration();
}

void AudioPlayer::setVolume(double volume)
{
    m_output->setVolume(qBound(0.0, volume, 1.0));
}

double AudioPlayer::volume() const
{
    return m_output->volume();
}

void AudioPlayer::setMuted(bool muted)
{
    m_output->setMuted(muted);
}

AudioPlayer::State AudioPlayer::state() const
{
    switch (m_player->playbackState()) {
    case QMediaPlayer::PlayingState: return State::Playing;
    case QMediaPlayer::PausedState: return State::Paused;
    default: return State::Stopped;
    }
}

} // namespace phonio
