#pragma once

#include "core/Reference.h"
#include "core/SearchResult.h"

#include <QListWidget>
#include <QPushButton>
#include <QWidget>

#include <functional>
#include <vector>

class Bible;
class BookTree;
class VersePanel;
class BookmarkStorage;
class HistoryStorage;

class Sidebar : public QWidget {
    Q_OBJECT
public:
    explicit Sidebar(QWidget* parent = nullptr);

    void setBibles(const std::vector<Bible*>& bibles);
    void setBookmarkStorage(BookmarkStorage* storage);
    void setHistoryStorage(HistoryStorage* storage);
    void refreshBookmarks();
    void refreshHistory();
    void displayVerse(const SearchResult& verse);

    void setRandomProvider(std::function<void()> provider);
    void setDailyProvider(std::function<void()> provider);

signals:
    void referenceSelected(const Reference& ref);
    void searchRequested(const std::string& query);

private:
    void setupUI();
    void updateSection(int index, bool expanded);

    BookTree* bookTree_;
    QListWidget* bookmarkList_;
    QListWidget* historyList_;
    VersePanel* versePanel_;

    QPushButton* booksToggle_;
    QPushButton* bookmarksToggle_;
    QPushButton* historyToggle_;
    QPushButton* verseToggle_;

    QWidget* booksSection_;
    QWidget* bookmarksSection_;
    QWidget* historySection_;
    QWidget* verseSection_;

    BookmarkStorage* bookmarkStorage_ = nullptr;
    HistoryStorage* historyStorage_ = nullptr;
};
