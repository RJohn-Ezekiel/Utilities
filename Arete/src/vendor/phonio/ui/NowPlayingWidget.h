#pragma once

#include "core/Types.h"

#include <QWidget>

class QLabel;
class QSlider;
class QToolButton;
class QGridLayout;

namespace phonio {

class PlaybackController;
class ArtworkManager;
class LyricsManager;
class ArtworkLabel;
class LyricsWidget;

// Full-page Now Playing view: large artwork, detailed metadata,
// large seek bar and synchronized lyrics.
class NowPlayingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NowPlayingWidget(PlaybackController* controller, ArtworkManager* artwork,
                              LyricsManager* lyrics, QWidget* parent = nullptr);

    void reloadArtwork();

signals:
    void backRequested();

private slots:
    void onCurrentTrackChanged(const Track& track);
    void onPositionChanged(qint64 positionMs);
    void onDurationChanged(qint64 durationMs);
    void onPlaybackStateChanged();
    void onLyricsLoaded();

private:
    void populateMetaGrid(const Track& track);
    QToolButton* makeIconButton(const QIcon& icon, const QString& tooltip);
    void updateBigSeek(qint64 positionMs);

    PlaybackController* m_controller;
    ArtworkManager* m_artwork;
    LyricsManager* m_lyrics;

    ArtworkLabel* m_artworkLabel;
    QLabel* m_title;
    QLabel* m_artist;
    QLabel* m_album;
    QLabel* m_metaSummary;
    QGridLayout* m_metaGrid;
    QWidget* m_metaContainer;
    QVector<QLabel*> m_metaLabels;
    LyricsWidget* m_lyricsWidget;
    QWidget* m_lyricsContainer;
    QToolButton* m_backButton;
    QSlider* m_seekSlider;
    QLabel* m_currentTime;
    QLabel* m_remainingTime;
    QToolButton* m_playPause;
    bool m_seeking = false;
    bool m_wasPlayingBeforeSeek = false;
};

} // namespace phonio
