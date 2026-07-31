#pragma once

#include "core/Types.h"

#include <QWidget>

namespace phonio {

class QueueManager;
class PlaybackController;
class LibraryManager;
class SongTableView;

// Queue page: shows the playback queue with reorder, remove and move controls.
// The queue itself is persistent; this page only reflects it.
class QueuePage : public QWidget
{
    Q_OBJECT

public:
    explicit QueuePage(QueueManager* queue, PlaybackController* controller,
                       LibraryManager* library, QWidget* parent = nullptr);

signals:
    void editMetadataRequested(const Track& track);
    void attachLyricsRequested(const Track& track);

private slots:
    void onQueueChanged();
    void onCurrentTrackChanged(const Track& track);
    void onLibraryTrackChanged(qint64 trackId);

public:
    void refreshTracks();

    QueueManager* m_queue;
    PlaybackController* m_controller;
    LibraryManager* m_library;
    SongTableView* m_table;
};

} // namespace phonio
