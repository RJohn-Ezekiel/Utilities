#include "ui/tagspanel.h"
#include "ui/style.h"

#include <QLabel>

namespace codex {

TagsPanel::TagsPanel(QWidget *parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(4);

    auto *header = new QLabel(QStringLiteral("Tags"));
    header->setStyleSheet(QStringLiteral(
        "font-weight: bold; color: %1; padding: 2px 0;"
    ).arg(style::ACCENT));
    m_layout->addWidget(header);

    m_tagInput = new QLineEdit;
    m_tagInput->setPlaceholderText(QStringLiteral("+ add tag..."));
    m_tagInput->setStyleSheet(QStringLiteral(
        "QLineEdit { background: #252525; color: #D8D8D8; border: 1px solid #333; "
        "border-radius: 3px; padding: 3px 6px; font-size: 12px; }"
    ));
    connect(m_tagInput, &QLineEdit::returnPressed, this, &TagsPanel::onAddTag);
    m_layout->addWidget(m_tagInput);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { border: none; background: transparent; }"));

    m_tagContainer = new QWidget;
    m_tagLayout = new QVBoxLayout(m_tagContainer);
    m_tagLayout->setContentsMargins(0, 0, 0, 0);
    m_tagLayout->setSpacing(2);
    m_tagLayout->addStretch();

    scroll->setWidget(m_tagContainer);
    m_layout->addWidget(scroll, 1);
}

void TagsPanel::showTags(const std::vector<std::string> &tags)
{
    // Clear existing tag buttons (keep input field)
    while (m_tagLayout->count() > 1) {
        auto *item = m_tagLayout->takeAt(0);
        if (item->widget())
            delete item->widget();
        delete item;
    }

    if (tags.empty()) {
        auto *emptyLabel = new QLabel(QStringLiteral("No tags"));
        emptyLabel->setStyleSheet(QStringLiteral("color: #666; font-size: 11px; padding: 4px 0;"));
        m_tagLayout->insertWidget(0, emptyLabel);
        return;
    }

    for (const auto &t : tags) {
        auto qTag = QString::fromStdString(t);
        auto *btn = new QPushButton(QStringLiteral("#%1").arg(qTag));
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { background: #252525; color: %1; border: none; "
            "padding: 3px 8px; border-radius: 3px; text-align: left; font-size: 12px; }"
            "QPushButton:hover { background: #333; }"
        ).arg(style::ACCENT));
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, &TagsPanel::onTagClick);
        m_tagLayout->insertWidget(m_tagLayout->count() - 1, btn);
    }
}

void TagsPanel::clear()
{
    while (m_tagLayout->count() > 1) {
        auto *item = m_tagLayout->takeAt(0);
        if (item->widget())
            delete item->widget();
        delete item;
    }
    m_tagInput->clear();
}

void TagsPanel::onTagClick()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    auto text = btn->text();
    if (text.startsWith(QChar('#')))
        text = text.mid(1);
    emit tagClicked(text.toStdString());
}

void TagsPanel::onAddTag()
{
    auto tag = m_tagInput->text().trimmed();
    if (tag.isEmpty()) return;
    if (tag.startsWith(QChar('#')))
        tag = tag.mid(1);
    // Remove spaces and special chars
    QString clean;
    for (const QChar &c : tag) {
        if (c.isLetterOrNumber() || c == QChar('-') || c == QChar('_') || c == QChar('/'))
            clean += c;
    }
    if (!clean.isEmpty()) {
        emit tagAddRequested(clean.toStdString());
        m_tagInput->clear();
    }
}

} // namespace codex
