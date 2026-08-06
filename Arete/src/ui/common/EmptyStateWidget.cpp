#include "ui/common/EmptyStateWidget.h"

#include <QVBoxLayout>

namespace arete::ui::common {

EmptyStateWidget::EmptyStateWidget(const QString& icon, const QString& message, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(12);

    m_iconLabel = new QLabel(icon, this);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setStyleSheet(QStringLiteral("font-size: 32px; color: #4A4A4A;"));

    m_messageLabel = new QLabel(message, this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet(QStringLiteral("color: #7A7A7A; font-size: 13px;"));

    layout->addWidget(m_iconLabel);
    layout->addWidget(m_messageLabel);
}

void EmptyStateWidget::setMessage(const QString& message)
{
    m_messageLabel->setText(message);
}

void EmptyStateWidget::setIcon(const QString& icon)
{
    m_iconLabel->setText(icon);
}

} // namespace arete::ui::common
