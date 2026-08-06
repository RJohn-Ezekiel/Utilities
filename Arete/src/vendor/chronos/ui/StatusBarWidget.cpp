#include "StatusBarWidget.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QTimer>

namespace chronos {

StatusBarWidget::StatusBarWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(28);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(12);

    m_stateLabel = new QLabel(QStringLiteral("Idle"), this);
    m_stateLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 12px;")
                                    .arg(Theme::SecondaryText.name()));
    layout->addWidget(m_stateLabel);

    m_notificationLabel = new QLabel(this);
    m_notificationLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 12px;")
                                           .arg(Theme::Accent.name()));
    layout->addWidget(m_notificationLabel, 1);

    m_shortcutLabel = new QLabel(
        QStringLiteral("Space: Pause  |  Ctrl+N: New Task  |  Ctrl+Q: Quit"), this);
    m_shortcutLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 11px;")
                                       .arg(Theme::SecondaryText.name()));
    m_shortcutLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(m_shortcutLabel);
}

void StatusBarWidget::setStateText(const QString& text)
{
    m_stateLabel->setText(text);
}

void StatusBarWidget::setNotification(const QString& text, int timeoutMs)
{
    m_notificationLabel->setText(text);

    if (timeoutMs > 0) {
        auto* timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, timer]() {
            m_notificationLabel->clear();
            timer->deleteLater();
        });
        timer->start(timeoutMs);
    }
}

void StatusBarWidget::updateForState(TimerState state)
{
    switch (state) {
    case TimerState::Idle:
        setStateText(QStringLiteral("Idle"));
        break;
    case TimerState::Focusing:
        setStateText(QStringLiteral("Focusing"));
        break;
    case TimerState::ShortBreak:
        setStateText(QStringLiteral("Short Break"));
        break;
    case TimerState::LongBreak:
        setStateText(QStringLiteral("Long Break"));
        break;
    case TimerState::Paused:
        setStateText(QStringLiteral("Paused"));
        break;
    }
}

} // namespace chronos
