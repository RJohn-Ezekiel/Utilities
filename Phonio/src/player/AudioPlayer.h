#pragma once

#include <QObject>

class QMediaPlayer;
class QAudioOutput;

namespace phonio {

// Thin wrapper around QMediaPlayer/QAudioOutput.
// No playlist logic here - PlaybackController owns that.
class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    enum class State { Stopped, Playing, Paused };

    explicit AudioPlayer(QObject* parent = nullptr);

    bool playFile(const QString& filePath);
    void play();
    void pause();
    void toggle();
    void stop();

    void seek(qint64 positionMs);
    qint64 positionMs() const;
    qint64 durationMs() const;

    void setVolume(double volume);      // 0..1
    double volume() const;
    void setMuted(bool muted);

    State state() const;

signals:
    void stateChanged(State state);
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void volumeChanged(double volume);
    void mediaEnded();
    void playbackFailed(const QString& error);

private:
    QMediaPlayer* m_player;
    QAudioOutput* m_output;
};

} // namespace phonio
