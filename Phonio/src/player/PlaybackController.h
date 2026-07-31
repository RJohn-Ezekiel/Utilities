#pragma once

#include "core/Types.h"
#include "player/AudioPlayer.h"
#include "queue/QueueManager.h"

#include <QObject>
#include <QVector>
#include <QRandomGenerator>

namespace phonio {

class LibraryManager;
class SettingsManager;
class LyricsManager;

// Orchestrates playback: current track, shuffle/repeat, end-of-track
// advancement, play counting, playback position restore/save.
class PlaybackController : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackController(AudioPlayer* player, QueueManager* queue,
                                LibraryManager* library, SettingsManager* settings,
                                LyricsManager* lyrics, QObject* parent = nullptr);

    AudioPlayer* player() const { return m_player; }
    QueueManager* queue() const { return m_queue; }

    std::optional<Track> currentTrack() const;
    qint64 currentTrackId() const;

    // Plays the track (from library or id); enqueues if not present.
    void playTrack(const Track& track);
    void playTrackId(qint64 trackId);
    void playQueueIndex(int index);

    void playNext();
    void playPrevious();
    void togglePlayPause();

    void setShuffle(bool enabled);
    bool shuffleEnabled() const;
    void cycleRepeatMode();
    RepeatMode repeatMode() const;

    // Rebuilds the queue from the given tracks and starts playing `startIndex`.
    void playTracks(const QVector<Track>& tracks, int startIndex = 0);

    bool hasCurrentTrack() const;
    bool isPlaying() const;
    bool isPaused() const;

signals:
    void currentTrackChanged(const Track& track);
    void playbackStateChanged(AudioPlayer::State state);
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void shuffleChanged(bool enabled);
    void repeatModeChanged(RepeatMode mode);
    void playRequestFailed(const QString& error);

private slots:
    void onMediaEnded();
    void onTrackChanged(qint64 trackId);

private:
    void startTrack(const Track& track);
    std::optional<int> nextQueueIndex();
    void buildShuffleOrder();
    void persistPositionPeriodically(qint64 positionMs);
    void restoreSavedPosition(const Track& track);

    AudioPlayer* m_player;
    QueueManager* m_queue;
    LibraryManager* m_library;
    SettingsManager* m_settings;
    LyricsManager* m_lyrics;

    bool m_shuffle = false;
    RepeatMode m_repeat = RepeatMode::Off;
    QVector<int> m_shuffleOrder;        // permutation of queue indices
    int m_shufflePosition = 0;
    qint64 m_positionSaveAccumulator = 0;
    qint64 m_lastPositionMs = 0;
};

} // namespace phonio
