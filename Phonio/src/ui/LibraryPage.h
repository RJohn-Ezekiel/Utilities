#pragma once

#include "core/Types.h"

#include <QWidget>

class QLineEdit;
class QComboBox;
class QLabel;

namespace phonio {

class LibraryManager;
class PlaybackController;
class SongTableView;
class PlaylistManager;

// Library page: search box, sort dropdown, track table with context actions.
class LibraryPage : public QWidget
{
    Q_OBJECT

public:
    explicit LibraryPage(LibraryManager* library, PlaybackController* controller,
                         PlaylistManager* playlists, QWidget* parent = nullptr);

    void refresh();
    void highlightTrack(qint64 trackId);

signals:
    void editMetadataRequested(const Track& track);
    void attachLyricsRequested(const Track& track);

private slots:
    void onLibraryChanged();
    void onTrackChanged(qint64 trackId);
    void onTrackRemoved(qint64 trackId);

private:
    LibraryManager* m_library;
    PlaybackController* m_controller;
    SongTableView* m_table;
    QLineEdit* m_searchBox;
    QComboBox* m_sortBox;
    QLabel* m_countLabel;
};

} // namespace phonio
