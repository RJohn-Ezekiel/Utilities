#include "ui/searchbar.h"

namespace codex {

SearchBar::SearchBar(QWidget *parent)
    : QLineEdit(parent)
{
    setPlaceholderText(QStringLiteral("Search notes..."));
    setClearButtonEnabled(true);

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setInterval(300);
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, [this]() {
        emit searchRequested(text().trimmed().toStdString());
    });

    connect(this, &QLineEdit::textChanged, this, &SearchBar::onTextChanged);
}

void SearchBar::onTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        m_debounceTimer->stop();
        emit searchRequested(std::string());
    } else {
        m_debounceTimer->start();
    }
}

} // namespace codex
