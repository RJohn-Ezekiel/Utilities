#include "ui/ArtistsPage.h"

#include "ui/SongTableView.h"
#include "library/LibraryManager.h"
#include "player/PlaybackController.h"
#include "artwork/ArtworkManager.h"

#include <QListWidget>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QMetaType>

namespace phonio {

// ---------------------------------------------------------------------------
// BrowsePage
// ---------------------------------------------------------------------------

BrowsePage::BrowsePage(LibraryManager* library, PlaybackController* controller,
                       ArtworkManager* artwork, QWidget* parent)
    : QWidget(parent)
    , m_library(library)
    , m_controller(controller)
    , m_artwork(artwork)
    , m_list(new QListWidget(this))
    , m_pageTitle(new QLabel(this))
    , m_countLabel(new QLabel(this))
    , m_searchBox(new QLineEdit(this))
    , m_stack(new QStackedWidget(this))
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 16);
    root->setSpacing(14);

    auto* titleRow = new QHBoxLayout;
    m_pageTitle->setObjectName(QStringLiteral("pageTitle"));
    m_countLabel->setObjectName(QStringLiteral("pageSubtitle"));
    titleRow->addWidget(m_pageTitle);
    titleRow->addSpacing(10);
    titleRow->addWidget(m_countLabel, 0, Qt::AlignBottom);
    titleRow->addStretch();
    root->addLayout(titleRow);

    auto* toolbar = new QHBoxLayout;
    m_searchBox->setPlaceholderText(tr("Filter..."));
    m_searchBox->setClearButtonEnabled(true);
    m_searchBox->setFixedWidth(280);
    toolbar->addWidget(m_searchBox);
    toolbar->addStretch();
    root->addLayout(toolbar);

    m_list->setObjectName(QStringLiteral("browseList"));
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_stack->addWidget(m_list);
    root->addWidget(m_stack, 1);

    connect(m_searchBox, &QLineEdit::textChanged, this, [this](const QString& text) {
        for (int i = 0; i < m_list->count(); ++i) {
            const bool visible = m_list->item(i)->text().contains(text, Qt::CaseInsensitive);
            m_list->item(i)->setHidden(!visible);
        }
    });
    connect(m_list, &QListWidget::itemActivated, this, &BrowsePage::onItemActivated);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &BrowsePage::onItemActivated);
}

void BrowsePage::setGroups(const QVector<Group>& groups, const QString& emptyMessage)
{
    m_groups = groups;
    m_list->clear();
    for (const auto& group : groups) {
        auto* item = new QListWidgetItem(group.name, m_list);
        item->setToolTip(group.subtitle);
        item->setData(Qt::UserRole, QVariant::fromValue(group));
    }
    m_list->setEnabled(!groups.isEmpty());
    m_countLabel->setText(groups.isEmpty() ? emptyMessage : tr("%1 entries").arg(groups.size()));
    if (m_stack->currentWidget() == m_list)
        m_stack->setCurrentWidget(m_list);
}

void BrowsePage::refresh()
{
    // Re-apply search filter after library changes.
    m_searchBox->textChanged(m_searchBox->text());
}

void BrowsePage::addDetailTable(SongTableView* table)
{
    m_detailTable = table;
    auto* detailPage = new QWidget(this);
    auto* layout = new QVBoxLayout(detailPage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(table);
    m_stack->addWidget(detailPage);
}

void BrowsePage::onItemActivated(QListWidgetItem* item)
{
    if (!item)
        return;
    const Group group = item->data(Qt::UserRole).value<Group>();
    if (!group.trackIds.isEmpty())
        onGroupActivated(group);
}

// ---------------------------------------------------------------------------
// ArtistsPage
// ---------------------------------------------------------------------------

void ArtistsPage::setArtists(const QVector<Artist>& artists)
{
    QVector<Group> groups;
    groups.reserve(artists.size());
    for (const auto& artist : artists) {
        Group group;
        group.name = artist.name;
        group.subtitle = tr("%1 songs").arg(artist.trackIds.size());
        group.trackIds = artist.trackIds;
        groups.append(group);
    }
    m_pageTitle->setText(tr("Artists"));
    setGroups(groups, tr("No artists yet. Add music folders in Settings."));
}

void ArtistsPage::onGroupActivated(const Group& group)
{
    m_controller->playTracks(m_library->tracksForIds(group.trackIds), 0);
}

// ---------------------------------------------------------------------------
// AlbumsPage
// ---------------------------------------------------------------------------

void AlbumsPage::setAlbums(const QVector<Album>& albums)
{
    QVector<Group> groups;
    groups.reserve(albums.size());
    for (const auto& album : albums) {
        Group group;
        group.name = album.name;
        group.subtitle = album.year > 0 ? QStringLiteral("%1 · %2 songs").arg(album.year).arg(album.trackIds.size())
                                        : tr("%1 songs").arg(album.trackIds.size());
        group.trackIds = album.trackIds;
        groups.append(group);
    }
    m_pageTitle->setText(tr("Albums"));
    setGroups(groups, tr("No albums yet. Add music folders in Settings."));
}

void AlbumsPage::onGroupActivated(const Group& group)
{
    m_controller->playTracks(m_library->tracksForIds(group.trackIds), 0);
}

// ---------------------------------------------------------------------------
// GenresPage
// ---------------------------------------------------------------------------

void GenresPage::setGenres(const QVector<Genre>& genres)
{
    QVector<Group> groups;
    groups.reserve(genres.size());
    for (const auto& genre : genres) {
        Group group;
        group.name = genre.name;
        group.subtitle = tr("%1 songs").arg(genre.trackIds.size());
        group.trackIds = genre.trackIds;
        groups.append(group);
    }
    m_pageTitle->setText(tr("Genres"));
    setGroups(groups, tr("No genres yet."));
}

void GenresPage::onGroupActivated(const Group& group)
{
    m_controller->playTracks(m_library->tracksForIds(group.trackIds), 0);
}

} // namespace phonio
