#include "player/PlaybackController.h"

#include "library/LibraryManager.h"
#include "settings/SettingsManager.h"
#include "lyrics/LyricsManager.h"

#include <QTimer>
#include <algorithm>

namespace phonio {

namespace {
constexpr qint64 kPositionSaveIntervalMs = 5000;
constexpr qint64 kMinRestorePositionMs = 30000;   // only resume if we left off > 30s in
constexpr qint64 kMaxRestoreFractionMs = 90;      // and not past 90% of the track
}

PlaybackController::PlaybackController(AudioPlayer* player, QueueManager* queue,
                                       LibraryManager* library, SettingsManager* settings,
                                       LyricsManager* lyrics, QObject* parent)
    : QObject(parent)
    , m_player(player)
    , m_queue(queue)
    , m_library(library)
    , m_settings(settings)
    , m_lyrics(lyrics)
    , m_shuffle(m_settings->isShuffleEnabled())
    , m_repeat(static_cast<RepeatMode>(m_settings->repeatMode()))
{
    connect(m_player, &AudioPlayer::mediaEnded, this, &PlaybackController::onMediaEnded);
    connect(m_player, &AudioPlayer::stateChanged, this, &PlaybackController::playbackStateChanged);
    connect(m_player, &AudioPlayer::positionChanged, this, &PlaybackController::persistPositionPeriodically);
    connect(m_player, &AudioPlayer::positionChanged, this, &PlaybackController::positionChanged);
    connect(m_player, &AudioPlayer::durationChanged, this, &PlaybackController::durationChanged);
    connect(m_player, &AudioPlayer::playbackFailed, this, &PlaybackController::playRequestFailed);
    connect(m_library, &LibraryManager::trackRemoved, this, &PlaybackController::onTrackChanged);
    connect(m_library, &LibraryManager::trackChanged, this, &PlaybackController::onTrackChanged);
}

bool PlaybackController::hasCurrentTrack() const
{
    return currentTrackId() >= 0;
}

qint64 PlaybackController::currentTrackId() const
{
    return m_queue->currentTrackId().value_or(-1);
}

std::optional<Track> PlaybackController::currentTrack() const
{
    const qint64 id = currentTrackId();
    if (id < 0)
        return std::nullopt;
    return m_library->trackById(id);
}

bool PlaybackController::isPlaying() const
{
    return m_player->state() == AudioPlayer::State::Playing;
}

bool PlaybackController::isPaused() const
{
    return m_player->state() == AudioPlayer::State::Paused;
}

void PlaybackController::startTrack(const Track& track)
{
    if (!track.isValid())
        return;
    m_player->playFile(track.filePath);
    m_queue->setCurrentTrackId(track.id);
    restoreSavedPosition(track);
    m_lyrics->loadLyricsFor(track);
    emit currentTrackChanged(track);
}

void PlaybackController::playTrack(const Track& track)
{
    if (!track.isValid())
        return;
    const qint64 id = track.id;
    if (!m_queue->trackIds().contains(id))
        m_queue->enqueue({id});
    startTrack(track);
}

void PlaybackController::playTrackId(qint64 trackId)
{
    const auto track = m_library->trackById(trackId);
    if (track)
        playTrack(*track);
}

void PlaybackController::playQueueIndex(int index)
{
    const qint64 id = m_queue->trackIdAt(index);
    if (id < 0)
        return;
    const auto track = m_library->trackById(id);
    if (track) {
        m_queue->setCurrentIndex(index);
        startTrack(*track);
    }
}

void PlaybackController::playTracks(const QVector<Track>& tracks, int startIndex)
{
    if (tracks.isEmpty())
        return;
    QVector<qint64> ids;
    ids.reserve(tracks.size());
    for (const auto& track : tracks)
        ids.append(track.id);
    m_queue->clear();
    m_queue->enqueue(ids);
    m_queue->setCurrentIndex(startIndex);
    if (m_shuffle)
        buildShuffleOrder();
    startTrack(tracks.at(startIndex));
}

void PlaybackController::playNext()
{
    if (m_repeat == RepeatMode::One) {
        if (const auto track = currentTrack())
            startTrack(*track);
        return;
    }
    const auto next = nextQueueIndex();
    if (!next)
        return;
    playQueueIndex(*next);
}

void PlaybackController::playPrevious()
{
    // Restart the current track if we are > 3 seconds in.
    if (m_player->positionMs() > 3000 && hasCurrentTrack()) {
        m_player->seek(0);
        return;
    }
    const int current = m_queue->currentIndex();
    if (current > 0)
        playQueueIndex(current - 1);
}

std::optional<int> PlaybackController::nextQueueIndex()
{
    if (m_queue->isEmpty())
        return std::nullopt;

    if (m_shuffle && m_shuffleOrder.size() > 1) {
        if (m_shufflePosition + 1 < m_shuffleOrder.size())
            return m_shuffleOrder.at(m_shufflePosition + 1);
        if (m_repeat == RepeatMode::All) {
            m_shufflePosition = 0;
            return m_shuffleOrder.first();
        }
        return std::nullopt;
    }

    const int current = m_queue->currentIndex();
    if (current + 1 < m_queue->size())
        return current + 1;
    if (m_repeat == RepeatMode::All && !m_queue->isEmpty())
        return 0;
    return std::nullopt;
}

void PlaybackController::onMediaEnded()
{
    if (const auto track = currentTrack())
        m_library->incrementPlayCount(*track);
    playNext();
}

void PlaybackController::onTrackChanged(qint64 trackId)
{
    if (trackId == currentTrackId()) {
        m_queue->setCurrentIndex(m_queue->trackIds().indexOf(trackId));
        if (const auto track = currentTrack())
            emit currentTrackChanged(*track);
    }
}

void PlaybackController::togglePlayPause()
{
    if (!hasCurrentTrack())
        return;
    m_player->toggle();
}

void PlaybackController::setShuffle(bool enabled)
{
    if (m_shuffle == enabled)
        return;
    m_shuffle = enabled;
    m_settings->setShuffleEnabled(enabled);
    if (m_shuffle)
        buildShuffleOrder();
    emit shuffleChanged(enabled);
}

bool PlaybackController::shuffleEnabled() const
{
    return m_shuffle;
}

void PlaybackController::cycleRepeatMode()
{
    switch (m_repeat) {
    case RepeatMode::Off: m_repeat = RepeatMode::All; break;
    case RepeatMode::All: m_repeat = RepeatMode::One; break;
    case RepeatMode::One: m_repeat = RepeatMode::Off; break;
    }
    m_settings->setRepeatMode(static_cast<int>(m_repeat));
    emit repeatModeChanged(m_repeat);
}

RepeatMode PlaybackController::repeatMode() const
{
    return m_repeat;
}

void PlaybackController::buildShuffleOrder()
{
    const int n = m_queue->size();
    m_shuffleOrder.resize(n);
    for (int i = 0; i < n; ++i)
        m_shuffleOrder[i] = i;
    for (int i = n - 1; i > 0; --i) {
        const int j = QRandomGenerator::global()->bounded(i + 1);
        std::swap(m_shuffleOrder[i], m_shuffleOrder[j]);
    }
    const int current = m_queue->currentIndex();
    if (current >= 0) {
        const int pos = m_shuffleOrder.indexOf(current);
        if (pos >= 0)
            std::swap(m_shuffleOrder[0], m_shuffleOrder[pos]);
    }
    m_shufflePosition = 0;
}

void PlaybackController::persistPositionPeriodically(qint64 positionMs)
{
    if (!hasCurrentTrack() || !m_settings->rememberPosition())
        return;
    m_positionSaveAccumulator += qAbs(positionMs - m_lastPositionMs);
    m_lastPositionMs = positionMs;
    if (m_positionSaveAccumulator >= kPositionSaveIntervalMs) {
        m_positionSaveAccumulator = 0;
        m_settings->savePositionForTrack(currentTrackId(), positionMs);
    }
}

void PlaybackController::restoreSavedPosition(const Track& track)
{
    if (!m_settings->rememberPosition())
        return;
    const qint64 saved = m_settings->savedPositionForTrack(track.id);
    const qint64 duration = track.durationMs > 0 ? track.durationMs : m_player->durationMs();
    if (saved > kMinRestorePositionMs && duration > 0
        && saved < (duration * kMaxRestoreFractionMs) / 100) {
        QTimer::singleShot(150, this, [this, saved] { m_player->seek(saved); });
    }
}

} // namespace phonio
