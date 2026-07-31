#pragma once

#include "core/Types.h"

#include <QWidget>

class QListWidget;
class QLabel;

namespace phonio {

class PlaylistManager;
class PlaybackController;
class SongTableView;

// Playlists page: playlist navigation on the left, its tracks on the right.
// Supports create/rename/delete and drag & drop from the library.
class PlaylistsPage : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistsPage(PlaylistManager* playlists, PlaybackController* controller,
                           QWidget* parent = nullptr);

signals:
    void editMetadataRequested(const Track& track);
    void attachLyricsRequested(const Track& track);

private slots:
    void onPlaylistsChanged();
    void onPlaylistContentChanged(qint64 playlistId);
    void selectPlaylist(qint64 playlistId);

private:
    void loadPlaylist(qint64 playlistId);

private:
    PlaylistManager* m_playlists;
    PlaybackController* m_controller;
    QListWidget* m_nav;
    SongTableView* m_table;
    QLabel* m_title;
    QLabel* m_countLabel;
    qint64 m_selectedPlaylistId = -1;
};

} // namespace phonio
