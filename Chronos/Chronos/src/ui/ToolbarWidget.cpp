#include "ToolbarWidget.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QLabel>

namespace chronos {

ToolbarWidget::ToolbarWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(42);
    setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: transparent;"
        "  color: %1;"
        "  border: none;"
        "  padding: 4px 14px;"
        "  font-size: 13px;"
        "  font-weight: normal;"
        "}"
        "QPushButton:hover { background: %2; }"
        "QPushButton:pressed { background: %3; }"
        "QPushButton:disabled { color: %4; }"
    )
        .arg(Theme::PrimaryText.name())
        .arg(Theme::Hover.name())
        .arg(Theme::Selection.name())
        .arg(Theme::SecondaryText.name()));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 8, 0);
    layout->setSpacing(0);

    // App title
    auto* title = new QLabel(QStringLiteral("Chronos"), this);
    title->setStyleSheet(QStringLiteral("color: %1; font-size: 14px; font-weight: bold; padding: 0 16px 0 4px;")
                             .arg(Theme::Accent.name()));
    layout->addWidget(title);

    layout->addSpacing(12);

    m_startBtn = new QPushButton(QStringLiteral("Start"), this);
    m_startBtn->setToolTip(QStringLiteral("Start a focus session"));
    layout->addWidget(m_startBtn);

    m_pauseBtn = new QPushButton(QStringLiteral("Pause"), this);
    m_pauseBtn->setToolTip(QStringLiteral("Pause the current session"));
    m_pauseBtn->setVisible(false);
    layout->addWidget(m_pauseBtn);

    m_resumeBtn = new QPushButton(QStringLiteral("Resume"), this);
    m_resumeBtn->setToolTip(QStringLiteral("Resume the paused session"));
    m_resumeBtn->setVisible(false);
    layout->addWidget(m_resumeBtn);

    m_stopBtn = new QPushButton(QStringLiteral("Stop"), this);
    m_stopBtn->setToolTip(QStringLiteral("Stop and discard the current session"));
    m_stopBtn->setEnabled(false);
    layout->addWidget(m_stopBtn);

    m_skipBreakBtn = new QPushButton(QStringLiteral("Skip Break"), this);
    m_skipBreakBtn->setToolTip(QStringLiteral("Skip the current break"));
    m_skipBreakBtn->setEnabled(false);
    layout->addWidget(m_skipBreakBtn);

    layout->addStretch();

    m_miniModeBtn = new QPushButton(QStringLiteral("\u25A2 Mini"), this);
    m_miniModeBtn->setToolTip(QStringLiteral("Switch to compact mini mode"));
    m_miniModeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        "  padding: 4px 14px; font-size: 12px; }"
        "QPushButton:hover { background: %4; color: %5; }"
    ).arg(Theme::Card.name()).arg(Theme::SecondaryText.name())
     .arg(Theme::Border.name()).arg(Theme::Hover.name())
     .arg(Theme::PrimaryText.name()));
    layout->addWidget(m_miniModeBtn);

    m_settingsBtn = new QPushButton(QStringLiteral("Settings"), this);
    m_settingsBtn->setToolTip(QStringLiteral("Open settings"));
    layout->addWidget(m_settingsBtn);

    connect(m_startBtn, &QPushButton::clicked, this, &ToolbarWidget::startClicked);
    connect(m_pauseBtn, &QPushButton::clicked, this, &ToolbarWidget::pauseClicked);
    connect(m_resumeBtn, &QPushButton::clicked, this, &ToolbarWidget::resumeClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &ToolbarWidget::stopClicked);
    connect(m_skipBreakBtn, &QPushButton::clicked, this, &ToolbarWidget::skipBreakClicked);
    connect(m_miniModeBtn, &QPushButton::clicked, this, &ToolbarWidget::miniModeClicked);
    connect(m_settingsBtn, &QPushButton::clicked, this, &ToolbarWidget::settingsClicked);
}

void ToolbarWidget::updateForState(TimerState state)
{
    switch (state) {
    case TimerState::Idle:
        m_startBtn->setEnabled(true);
        m_pauseBtn->setVisible(false);
        m_resumeBtn->setVisible(false);
        m_stopBtn->setEnabled(false);
        m_skipBreakBtn->setEnabled(false);
        break;
    case TimerState::Focusing:
    case TimerState::ShortBreak:
    case TimerState::LongBreak:
        m_startBtn->setEnabled(false);
        m_pauseBtn->setVisible(true);
        m_resumeBtn->setVisible(false);
        m_stopBtn->setEnabled(true);
        m_skipBreakBtn->setEnabled(state != TimerState::Focusing);
        break;
    case TimerState::Paused:
        m_startBtn->setEnabled(false);
        m_pauseBtn->setVisible(false);
        m_resumeBtn->setVisible(true);
        m_stopBtn->setEnabled(true);
        m_skipBreakBtn->setEnabled(false);
        break;
    }
}

} // namespace chronos
