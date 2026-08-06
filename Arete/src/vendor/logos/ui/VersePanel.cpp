#include "VersePanel.h"
#include "Theme.h"

#include <QVBoxLayout>

VersePanel::VersePanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void VersePanel::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(4);

    randomBtn_ = new QPushButton("Random Verse");
    dailyBtn_ = new QPushButton("Daily Verse");

    verseLabel_ = new QLabel;
    verseLabel_->setWordWrap(true);
    verseLabel_->setStyleSheet(QStringLiteral(
        "color:%1; font-size:12px; padding:4px;"
    ).arg(Theme::primaryText.name()));
    verseLabel_->setTextFormat(Qt::RichText);
    verseLabel_->setVisible(false);

    layout->addWidget(randomBtn_);
    layout->addWidget(dailyBtn_);
    layout->addWidget(verseLabel_);

    connect(randomBtn_, &QPushButton::clicked, this, [this]() {
        if (randomProvider_)
            randomProvider_();
    });

    connect(dailyBtn_, &QPushButton::clicked, this, [this]() {
        if (dailyProvider_)
            dailyProvider_();
    });

    connect(verseLabel_, &QLabel::linkActivated, this, [this](const QString& link) {
        auto parts = link.split('|');
        if (parts.size() == 3) {
            emit verseClicked(
                parts[0].toStdString(),
                parts[1].toInt(),
                parts[2].toInt()
            );
        }
    });
}

void VersePanel::setRandomProvider(VerseProvider provider)
{
    randomProvider_ = std::move(provider);
}

void VersePanel::setDailyProvider(VerseProvider provider)
{
    dailyProvider_ = std::move(provider);
}

void VersePanel::displayVerse(const SearchResult& verse)
{
    currentVerse_ = verse;

    QString html = QStringLiteral(
        "<a href='%1|%2|%3' style='color:%4; text-decoration:none;'>"
        "%5 %6:%7</a><br>"
        "<span style='color:%8;'>%9</span>"
    )
        .arg(QString::fromStdString(verse.bookName))
        .arg(verse.chapter)
        .arg(verse.verse)
        .arg(Theme::secondaryText.name())
        .arg(QString::fromStdString(verse.bookName))
        .arg(verse.chapter)
        .arg(verse.verse)
        .arg(Theme::primaryText.name())
        .arg(QString::fromStdString(verse.text));

    verseLabel_->setText(html);
    verseLabel_->setVisible(true);
}
