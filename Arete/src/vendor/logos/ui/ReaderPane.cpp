#include "ReaderPane.h"
#include "Theme.h"

#include <QScrollBar>

ReaderPane::ReaderPane(QWidget* parent)
    : QTextBrowser(parent)
{
    setOpenLinks(false);
    setReadOnly(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setupStyle();

    connect(this, &QTextBrowser::anchorClicked, this, [this](const QUrl& url) {
        Reference ref;
        auto parts = url.toString().split('|');
        if (parts.size() == 3) {
            ref.book = parts[0].toStdString();
            ref.chapter = parts[1].toInt();
            ref.verseStart = parts[2].toInt();
            currentRef_ = ref;
            emit referenceClicked(ref);
        }
    });
}

void ReaderPane::setupStyle()
{
    setStyleSheet(QStringLiteral(
        "QTextBrowser {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "}"
    ).arg(Theme::readingArea.name(), Theme::primaryText.name()));
}

void ReaderPane::displayChapter(const std::string& bookName, int chapter,
                                const std::string& translation)
{
    currentRef_ = {bookName, chapter, {}, {}};

    QString html;
    html += QStringLiteral(
        "<div style='font-size:%3px; line-height:%4; color:%1; "
        "font-family:%2;'>"
    )
        .arg(Theme::secondaryText.name())
        .arg("'JetBrains Mono', 'Cascadia Mono', 'Noto Sans Mono', monospace")
        .arg(16)
        .arg(1.8);

    html += QStringLiteral(
        "<h2 style='color:%1; font-weight:normal; margin-bottom:12px;'>"
        "%2 %3 <span style='font-size:12px; color:%4;'>%5</span></h2>"
    )
        .arg(Theme::primaryText.name())
        .arg(QString::fromStdString(bookName))
        .arg(chapter)
        .arg(Theme::secondaryText.name())
        .arg(QString::fromStdString(translation));

    // We'll lazy-load verses when setVerseText is called
    // This method just prepares the chapter context
    setHtml(html);
}

void ReaderPane::displayVerses(const std::string& bookName, int chapter,
                               int verseStart, int verseEnd,
                               const std::string& translation)
{
    currentRef_ = {bookName, chapter, verseStart, verseEnd};
}

void ReaderPane::setVerseText(const std::string& html)
{
    setHtml(QString::fromStdString(html));
}
