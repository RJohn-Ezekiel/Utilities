#pragma once

#include "core/Reference.h"

#include <QTextBrowser>
#include <QString>

#include <string>

class ReaderPane : public QTextBrowser {
    Q_OBJECT
public:
    explicit ReaderPane(QWidget* parent = nullptr);

    void displayChapter(const std::string& bookName, int chapter,
                        const std::string& translation);
    void displayVerses(const std::string& bookName, int chapter,
                       int verseStart, int verseEnd,
                       const std::string& translation);
    void setVerseText(const std::string& html);

    [[nodiscard]] Reference currentReference() const noexcept { return currentRef_; }

signals:
    void referenceClicked(const Reference& ref);

private:
    void setupStyle();
    Reference currentRef_;
};
