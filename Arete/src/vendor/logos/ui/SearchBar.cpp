#include "SearchBar.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QLabel>

SearchBar::SearchBar(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SearchBar::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);

    auto* icon = new QLabel("Search");
    icon->setStyleSheet(QStringLiteral("color:%1; font-size:11px; font-weight:bold;")
        .arg(Theme::secondaryText.name()));

    input_ = new QLineEdit;
    input_->setPlaceholderText("Search verses...");
    input_->setClearButtonEnabled(true);

    layout->addWidget(icon);
    layout->addWidget(input_, 1);

    connect(input_, &QLineEdit::returnPressed, this, [this]() {
        std::string query = input_->text().trimmed().toStdString();
        if (!query.empty()) {
            emit searchRequested(query);
            if (callback_)
                callback_(query);
        }
    });
}

void SearchBar::setSearchCallback(std::function<void(std::string)> callback)
{
    callback_ = std::move(callback);
}

void SearchBar::clear()
{
    input_->clear();
}
