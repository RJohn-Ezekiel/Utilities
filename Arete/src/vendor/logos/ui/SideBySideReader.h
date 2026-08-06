#pragma once

#include "ReaderPane.h"

#include <QScrollBar>
#include <QSplitter>

#include <string>
#include <vector>

class Bible;

class SideBySideReader : public QSplitter {
    Q_OBJECT
public:
    explicit SideBySideReader(QWidget* parent = nullptr);

    void setLeftBible(const Bible* bible);
    void setRightBible(const Bible* bible);

    void displayReference(const std::string& bookName, int chapter,
                          std::optional<int> verseStart = {},
                          std::optional<int> verseEnd = {});

    ReaderPane* leftPane() const { return left_; }
    ReaderPane* rightPane() const { return right_; }

signals:
    void referenceClicked(const Reference& ref);

private:
    void syncScrollBars();
    void renderPane(ReaderPane* pane, const Bible* bible,
                    const std::string& bookName, int chapter,
                    std::optional<int> verseStart,
                    std::optional<int> verseEnd);

    ReaderPane* left_;
    ReaderPane* right_;
    const Bible* leftBible_ = nullptr;
    const Bible* rightBible_ = nullptr;
    bool syncing_ = false;
};
