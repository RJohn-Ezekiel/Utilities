#pragma once

#include "data/HymnLibrary.h"

#include <QWidget>
#include <QString>

class QLineEdit;
class QListWidget;
class QSplitter;
class QTextBrowser;
class QPushButton;
class QLabel;
class QScrollBar;

// Hymn Mode: searchable hymn library rendered Latin | English side by side,
// with favourite/bookmark tracking and prev/next navigation.
class HymnModeWidget : public QWidget {
    Q_OBJECT
public:
    explicit HymnModeWidget(QWidget* parent = nullptr);

    void setLibrary(HymnLibrary* library);
    void setFavouriteStorage(std::function<bool(const QString&)> isFavourite,
                             std::function<void(const QString&, bool)> setFavourite);

    [[nodiscard]] int currentIndex() const;
    void showHymn(int index);
    void selectByText(const QString& text);

public slots:
    void refresh();

signals:
    void hymnSelected(int index);

private slots:
    void onSearchChanged(const QString& text);
    void onListSelectionChanged();
    void onPrev();
    void onNext();
    void onCopy();
    void onToggleFavourite();
    void onFontLarger();
    void onFontSmaller();
    void onNew();
    void onEdit();
    void onDelete();

private:
    void rebuildList();
    void updateReader();
    void updateStatus();
    void syncScroll(QScrollBar* from, QScrollBar* to);
    static int selectedRowById(const QVector<Hymn>& list, const QString& id);

    HymnLibrary* library_ = nullptr;
    QVector<Hymn> currentList_;

    std::function<bool(const QString&)> isFavourite_;
    std::function<void(const QString&, bool)> setFavourite_;

    QLineEdit* searchBox_;
    QListWidget* list_;
    QTextBrowser* latinView_;
    QTextBrowser* englishView_;
    QSplitter* readerSplitter_;
    QLabel* titleLabel_;
    QLabel* metaLabel_;
    QPushButton* prevBtn_;
    QPushButton* nextBtn_;
    QPushButton* copyBtn_;
    QPushButton* favBtn_;
    QPushButton* newBtn_;
    QPushButton* editBtn_;
    QPushButton* deleteBtn_;

    bool syncingScroll_ = false;
    int baseFontSize_ = 16;
};
