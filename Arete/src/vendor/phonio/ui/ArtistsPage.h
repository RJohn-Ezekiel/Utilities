#pragma once

#include "core/Types.h"

#include <QWidget>
#include <QVector>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QStackedWidget;
class QLineEdit;

namespace phonio {

class LibraryManager;
class PlaybackController;
class ArtworkManager;
class SongTableView;

// Base page for browsing grouped collections (artists/albums/genres).
// Shows a list of groups; double-click or "Play" plays all their tracks.
class BrowsePage : public QWidget
{
    Q_OBJECT

public:
    struct Group {
        QString name;
        QString subtitle;
        QVector<qint64> trackIds;
    };

    explicit BrowsePage(LibraryManager* library, PlaybackController* controller,
                        ArtworkManager* artwork, QWidget* parent = nullptr);

    void setGroups(const QVector<Group>& groups, const QString& emptyMessage);
    void refresh();

    void addDetailTable(SongTableView* table);

protected:
    virtual void onGroupActivated(const Group& group) = 0;

    LibraryManager* m_library;
    PlaybackController* m_controller;
    ArtworkManager* m_artwork;
    QListWidget* m_list;
    QLabel* m_pageTitle;
    QLabel* m_countLabel;
    QLineEdit* m_searchBox;
    QStackedWidget* m_stack;
    SongTableView* m_detailTable = nullptr;
    QVector<Group> m_groups;

private slots:
    void onItemActivated(QListWidgetItem* item);
};

// --- Artists -------------------------------------------------------------
class ArtistsPage : public BrowsePage
{
    Q_OBJECT
public:
    using BrowsePage::BrowsePage;
    void setArtists(const QVector<Artist>& artists);
protected:
    void onGroupActivated(const Group& group) override;
};

// --- Albums --------------------------------------------------------------
class AlbumsPage : public BrowsePage
{
    Q_OBJECT
public:
    using BrowsePage::BrowsePage;
    void setAlbums(const QVector<Album>& albums);
protected:
    void onGroupActivated(const Group& group) override;
};

// --- Genres --------------------------------------------------------------
class GenresPage : public BrowsePage
{
    Q_OBJECT
public:
    using BrowsePage::BrowsePage;
    void setGenres(const QVector<Genre>& genres);
protected:
    void onGroupActivated(const Group& group) override;
};

} // namespace phonio

Q_DECLARE_METATYPE(phonio::BrowsePage::Group)
