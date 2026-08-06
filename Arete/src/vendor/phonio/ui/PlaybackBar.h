#pragma once

#include "core/Types.h"

#include <QWidget>

class QLabel;
class QSlider;
class QToolButton;

namespace phonio {

class ArtworkLabel;
class PlaybackController;
class ArtworkManager;

// Bottom playback bar: artwork, track info, transport controls, seek slider,
// time labels and volume.
class PlaybackBar : public QWidget
{
    Q_OBJECT

public:
    explicit PlaybackBar(PlaybackController* controller, ArtworkManager* artwork,
                         QWidget* parent = nullptr);

signals:
    void artworkClicked();          // opens the Now Playing page

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onCurrentTrackChanged(const Track& track);
    void onPositionChanged(qint64 positionMs);
    void onDurationChanged(qint64 durationMs);
    void onPlaybackStateChanged();
    void onShuffleChanged(bool enabled);
    void onRepeatModeChanged(RepeatMode mode);

private:
    void setPlaying(bool playing);
    void updateRepeatIcon();
    QToolButton* makeTransportButton(const QIcon& icon, const QString& tooltip);

    PlaybackController* m_controller;
    ArtworkManager* m_artwork;

    QWidget* m_trackInfoArea;
    ArtworkLabel* m_art;
    QLabel* m_title;
    QLabel* m_artist;
    QToolButton* m_playPause;
    QToolButton* m_shuffle;
    QToolButton* m_repeat;
    QSlider* m_seekSlider;
    QLabel* m_currentTime;
    QLabel* m_totalTime;
    QSlider* m_volumeSlider;
    bool m_seeking = false;
    bool m_wasPlayingBeforeSeek = false;
};

} // namespace phonio
