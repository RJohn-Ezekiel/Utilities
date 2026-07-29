#include "Sidebar.h"
#include "BookTree.h"
#include "VersePanel.h"
#include "Theme.h"
#include "core/Bible.h"
#include "core/Reference.h"
#include "storage/BookmarkStorage.h"
#include "storage/HistoryStorage.h"

#include <QVBoxLayout>

Q_DECLARE_METATYPE(Reference)

Sidebar::Sidebar(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void Sidebar::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto makeToggle = [this](const QString& text) {
        auto* btn = new QPushButton(text);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: %1;"
            "  color: %2;"
            "  border: none;"
            "  border-bottom: 1px solid %3;"
            "  text-align: left;"
            "  padding: 6px 12px;"
            "  font-size: 11px;"
            "  font-weight: bold;"
            "  text-transform: uppercase;"
            "  letter-spacing: 1px;"
            "}"
            "QPushButton:hover { background-color: %4; }"
        ).arg(Theme::sidebar.name(), Theme::secondaryText.name(),
              Theme::borders.name(), Theme::hover.name()));
        return btn;
    };

    // Books section
    booksToggle_ = makeToggle("Books");
    booksSection_ = new QWidget;
    auto* booksLayout = new QVBoxLayout(booksSection_);
    booksLayout->setContentsMargins(0, 0, 0, 0);
    bookTree_ = new BookTree;
    booksLayout->addWidget(bookTree_);

    // Bookmarks section
    bookmarksToggle_ = makeToggle("Bookmarks");
    bookmarksSection_ = new QWidget;
    auto* bmLayout = new QVBoxLayout(bookmarksSection_);
    bmLayout->setContentsMargins(0, 0, 0, 0);
    bookmarkList_ = new QListWidget;
    bmLayout->addWidget(bookmarkList_);

    // History section
    historyToggle_ = makeToggle("History");
    historySection_ = new QWidget;
    auto* histLayout = new QVBoxLayout(historySection_);
    histLayout->setContentsMargins(0, 0, 0, 0);
    historyList_ = new QListWidget;
    histLayout->addWidget(historyList_);

    // Verse section
    verseToggle_ = makeToggle("Verse");
    verseSection_ = new QWidget;
    auto* verseLayout = new QVBoxLayout(verseSection_);
    verseLayout->setContentsMargins(0, 0, 0, 0);
    versePanel_ = new VersePanel;
    verseLayout->addWidget(versePanel_);

    // Add sections
    layout->addWidget(booksToggle_);
    layout->addWidget(booksSection_, 1);
    layout->addWidget(bookmarksToggle_);
    layout->addWidget(bookmarksSection_);
    layout->addWidget(historyToggle_);
    layout->addWidget(historySection_);
    layout->addWidget(verseToggle_);
    layout->addWidget(verseSection_);
    layout->addStretch();

    // Toggle sections
    auto connectToggle = [this](QPushButton* btn, QWidget* section) {
        connect(btn, &QPushButton::clicked, this, [this, btn, section]() {
            bool visible = !section->isVisible();
            section->setVisible(visible);
            btn->setProperty("expanded", visible);
            btn->style()->unpolish(btn);
            btn->style()->polish(btn);
        });
    };

    connectToggle(booksToggle_, booksSection_);
    connectToggle(bookmarksToggle_, bookmarksSection_);
    connectToggle(historyToggle_, historySection_);
    connectToggle(verseToggle_, verseSection_);

    // Connect signals
    connect(bookTree_, &BookTree::chapterSelected,
            this, &Sidebar::referenceSelected);

    connect(bookmarkList_, &QListWidget::itemClicked,
            this, [this](QListWidgetItem* item) {
        auto ref = item->data(Qt::UserRole).value<Reference>();
        emit referenceSelected(ref);
    });

    connect(historyList_, &QListWidget::itemClicked,
            this, [this](QListWidgetItem* item) {
        auto ref = item->data(Qt::UserRole).value<Reference>();
        emit referenceSelected(ref);
    });

    connect(versePanel_, &VersePanel::verseClicked,
            this, [this](const std::string& book, int chapter, int verse) {
        Reference ref;
        ref.book = book;
        ref.chapter = chapter;
        ref.verseStart = verse;
        emit referenceSelected(ref);
    });
}

void Sidebar::setBibles(const std::vector<Bible*>& bibles)
{
    if (!bibles.empty())
        bookTree_->populate(bibles[0]);
}

void Sidebar::setBookmarkStorage(BookmarkStorage* storage)
{
    bookmarkStorage_ = storage;
    refreshBookmarks();
}

void Sidebar::setHistoryStorage(HistoryStorage* storage)
{
    historyStorage_ = storage;
    refreshHistory();
}

void Sidebar::refreshBookmarks()
{
    bookmarkList_->clear();
    if (!bookmarkStorage_)
        return;

    auto bookmarks = bookmarkStorage_->all();
    for (const auto& ref : bookmarks) {
        auto* item = new QListWidgetItem(
            QString::fromStdString(ref.toString()));
        item->setData(Qt::UserRole, QVariant::fromValue(ref));
        bookmarkList_->addItem(item);
    }
}

void Sidebar::refreshHistory()
{
    historyList_->clear();
    if (!historyStorage_)
        return;

    auto entries = historyStorage_->recent(30);
    for (const auto& ref : entries) {
        auto* item = new QListWidgetItem(
            QString::fromStdString(ref.toString()));
        item->setData(Qt::UserRole, QVariant::fromValue(ref));
        historyList_->addItem(item);
    }
}

void Sidebar::displayVerse(const SearchResult& verse)
{
    versePanel_->displayVerse(verse);
}

void Sidebar::setRandomProvider(std::function<void()> provider)
{
    versePanel_->setRandomProvider(std::move(provider));
}

void Sidebar::setDailyProvider(std::function<void()> provider)
{
    versePanel_->setDailyProvider(std::move(provider));
}
