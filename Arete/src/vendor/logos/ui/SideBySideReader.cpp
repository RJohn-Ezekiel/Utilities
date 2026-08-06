#include "SideBySideReader.h"
#include "core/Bible.h"
#include "core/Reference.h"
#include "Theme.h"

#include <QTextBrowser>

namespace {

QString buildVerseHtml(const std::string& bookName, int chapter,
                       const Chapter& ch,
                       std::optional<int> verseStart,
                       std::optional<int> verseEnd,
                       const std::string& translation)
{
    QString html;
    html += QStringLiteral(
        "<div style='font-size:%1px; line-height:%2; color:%3; "
        "font-family:%4; padding:8px 12px;'>"
    )
        .arg(14)
        .arg(1.8)
        .arg(Theme::primaryText.name())
        .arg("'JetBrains Mono', 'Cascadia Mono', 'Noto Sans Mono', monospace");

    html += QStringLiteral(
        "<div style='color:%1; font-size:13px; margin-bottom:4px; "
        "padding-bottom:2px; border-bottom:1px solid %2;'>"
        "%3 %4 <span style='font-size:10px; color:%5;'>%6</span></div>"
    )
        .arg(Theme::primaryText.name())
        .arg(Theme::borders.name())
        .arg(QString::fromStdString(bookName))
        .arg(chapter)
        .arg(Theme::secondaryText.name())
        .arg(QString::fromStdString(translation));

    for (const auto& v : ch.verses) {
        if (verseStart.has_value() && v.number < *verseStart)
            continue;
        if (verseEnd.has_value() && v.number > *verseEnd)
            break;

        QString text = QString::fromStdString(v.text);
        html += QStringLiteral(
            "<span style='color:%1; font-weight:bold; font-size:12px;'>%2</span> "
            "<span>%3</span><br>"
        )
            .arg(Theme::secondaryText.name())
            .arg(v.number)
            .arg(text);
    }

    html += "</div>";
    return html;
}

} // namespace

SideBySideReader::SideBySideReader(QWidget* parent)
    : QSplitter(Qt::Horizontal, parent)
{
    setHandleWidth(1);
    setStyleSheet(QStringLiteral("QSplitter::handle { background-color: %1; }")
        .arg(Theme::borders.name()));

    left_ = new ReaderPane(this);
    right_ = new ReaderPane(this);

    addWidget(left_);
    addWidget(right_);

    // Sync scrolling
    connect(left_->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this](int value) {
        if (!syncing_) {
            syncing_ = true;
            right_->verticalScrollBar()->setValue(value);
            syncing_ = false;
        }
    });

    connect(right_->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this](int value) {
        if (!syncing_) {
            syncing_ = true;
            left_->verticalScrollBar()->setValue(value);
            syncing_ = false;
        }
    });

    connect(left_, &ReaderPane::referenceClicked,
            this, &SideBySideReader::referenceClicked);
    connect(right_, &ReaderPane::referenceClicked,
            this, &SideBySideReader::referenceClicked);
}

void SideBySideReader::setLeftBible(const Bible* bible)
{
    leftBible_ = bible;
}

void SideBySideReader::setRightBible(const Bible* bible)
{
    rightBible_ = bible;
}

void SideBySideReader::displayReference(
    const std::string& bookName, int chapter,
    std::optional<int> verseStart,
    std::optional<int> verseEnd)
{
    renderPane(left_, leftBible_, bookName, chapter, verseStart, verseEnd);
    renderPane(right_, rightBible_, bookName, chapter, verseStart, verseEnd);
}

void SideBySideReader::renderPane(
    ReaderPane* pane, const Bible* bible,
    const std::string& bookName, int chapter,
    std::optional<int> verseStart,
    std::optional<int> verseEnd)
{
    if (!bible) {
        pane->setVerseText("No translation loaded.");
        return;
    }

    const auto* book = bible->findBook(bookName);
    if (!book) {
        pane->setVerseText(QStringLiteral(
            "<div style='color:%1; padding:20px;'>Book not found: %2</div>"
        ).arg(Theme::secondaryText.name()).arg(QString::fromStdString(bookName)).toStdString());
        return;
    }

    const auto& ch = book->chapter(chapter);
    if (ch.verses.empty()) {
        pane->setVerseText(QStringLiteral(
            "<div style='color:%1; padding:20px;'>Chapter not found.</div>"
        ).arg(Theme::secondaryText.name()).toStdString());
        return;
    }

    std::string translation = bible->shortName();
    QString html = buildVerseHtml(bookName, chapter, ch,
                                  verseStart, verseEnd, translation);
    pane->setVerseText(html.toStdString());
    pane->displayVerses(bookName, chapter,
                        verseStart.value_or(1),
                        verseEnd.value_or(ch.verses.back().number),
                        translation);
}
