#include "services/MusicService.h"
#include "app/AppContext.h"
#include "app/App.h"
#include "library/LibraryManager.h"
#include "queue/QueueManager.h"
#include "player/PlaybackController.h"
#include "player/AudioPlayer.h"

namespace arete::services {

namespace phonio = ::phonio;

MusicService::MusicService(app::AppContext* context, QObject* parent)
    : QObject(parent), m_context(context)
{
    auto* controller = m_context->phonioApp()->controller();
    connect(controller, &phonio::PlaybackController::playbackStateChanged, this,
            [this] { emit playbackChanged(); });
    connect(controller, &phonio::PlaybackController::currentTrackChanged, this,
            [this] { emit playbackChanged(); });
}

bool MusicService::isPlaying() const
{
    return m_context->phonioApp()->controller()->isPlaying();
}

QString MusicService::currentTitle() const
{
    if (!m_context->phonioApp()->controller()->hasCurrentTrack()) return {};
    const phonio::Track t = m_context->phonioApp()->library()->trackById(
        m_context->phonioApp()->queue()->trackIdAt(
            m_context->phonioApp()->queue()->currentIndex())).value_or(phonio::Track{});
    return t.title.isEmpty() ? QStringLiteral("Unknown") : t.title;
}

QString MusicService::currentArtist() const
{
    if (!m_context->phonioApp()->controller()->hasCurrentTrack()) return {};
    const qint64 id = m_context->phonioApp()->queue()->trackIdAt(
        m_context->phonioApp()->queue()->currentIndex());
    return m_context->phonioApp()->library()->trackById(id).value_or(phonio::Track{}).artist;
}

int MusicService::volumePercent() const
{
    return static_cast<int>(m_context->phonioApp()->player()->volume() * 100.0);
}

bool MusicService::play(const QString& term)
{
    if (term.isEmpty()) return false;

    // Exact title first, then substring, then artist.
    const QVector<phonio::Track> tracks = m_context->phonioApp()->library()->tracks();
    int best = -1;
    for (int i = 0; i < tracks.size(); ++i) {
        if (tracks.at(i).title.compare(term, Qt::CaseInsensitive) == 0) { best = i; break; }
    }
    if (best < 0) {
        for (int i = 0; i < tracks.size(); ++i) {
            if (tracks.at(i).title.contains(term, Qt::CaseInsensitive)) { best = i; break; }
        }
    }
    if (best < 0) {
        for (int i = 0; i < tracks.size(); ++i) {
            if (tracks.at(i).artist.contains(term, Qt::CaseInsensitive)) { best = i; break; }
        }
    }
    if (best < 0) return false;

    m_context->phonioApp()->controller()->playTrack(tracks.at(best));
    return true;
}

void MusicService::playPause()
{
    m_context->phonioApp()->controller()->togglePlayPause();
}

void MusicService::pause()
{
    if (m_context->phonioApp()->controller()->isPlaying()) {
        m_context->phonioApp()->controller()->togglePlayPause();
    }
}

void MusicService::next()
{
    m_context->phonioApp()->controller()->playNext();
}

void MusicService::previous()
{
    m_context->phonioApp()->controller()->playPrevious();
}

void MusicService::setVolume(int percent)
{
    const double volume = qBound(0.0, percent / 100.0, 1.0);
    m_context->phonioApp()->player()->setVolume(volume);
    emit volumeChanged(percent);
}

void MusicService::toggleShuffle()
{
    auto* controller = m_context->phonioApp()->controller();
    controller->setShuffle(!controller->shuffleEnabled());
}

} // namespace arete::services
