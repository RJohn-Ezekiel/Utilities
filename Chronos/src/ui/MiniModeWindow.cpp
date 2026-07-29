#include "MiniModeWindow.h"
#include "CircularTimerWidget.h"
#include "services/TimerService.h"
#include "Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

namespace chronos {

MiniModeWindow::MiniModeWindow(TimerService* timerService, QWidget* parent)
    : QWidget(parent)
    , m_timerService(timerService)
{
    setWindowTitle(QStringLiteral("Chronos \u2014 Mini"));
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window);
    setFixedSize(220, 300);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setupUi();
    applyStyle();
}

void MiniModeWindow::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    // Title bar with restore button
    auto* titleRow = new QHBoxLayout();
    m_titleLabel = new QLabel(QStringLiteral("Chronos"), this);
    m_titleLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 11px; font-weight: bold; background: transparent;")
                                    .arg(Theme::Accent.name()));
    titleRow->addWidget(m_titleLabel);
    titleRow->addStretch();

    auto* restoreBtn = new QPushButton(QStringLiteral("\u2192"), this);
    restoreBtn->setFixedSize(20, 20);
    restoreBtn->setToolTip(QStringLiteral("Restore main window"));
    restoreBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: %1; border: none; font-size: 14px; }"
        "QPushButton:hover { color: %2; }"
    ).arg(Theme::SecondaryText.name()).arg(Theme::PrimaryText.name()));
    connect(restoreBtn, &QPushButton::clicked, this, &MiniModeWindow::restoreClicked);
    titleRow->addWidget(restoreBtn);
    root->addLayout(titleRow);

    // Circular timer (compact)
    m_circularTimer = new CircularTimerWidget(this);
    m_circularTimer->setFixedSize(160, 160);
    root->addWidget(m_circularTimer, 0, Qt::AlignCenter);

    // Task label
    m_taskLabel = new QLabel(this);
    m_taskLabel->setAlignment(Qt::AlignCenter);
    m_taskLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 10px; background: transparent;")
                                   .arg(Theme::SecondaryText.name()));
    m_taskLabel->setText(QStringLiteral("No task"));
    root->addWidget(m_taskLabel);

    // Pause/Resume buttons
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);
    btnRow->addStretch();

    const QString btnStyle = QStringLiteral(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        "  padding: 4px 14px; font-size: 11px; border-radius: 0px; }"
        "QPushButton:hover { background: %4; }"
        "QPushButton:disabled { color: %5; }"
    ).arg(Theme::Card.name()).arg(Theme::PrimaryText.name())
     .arg(Theme::Border.name()).arg(Theme::Hover.name())
     .arg(Theme::SecondaryText.name());

    m_pauseBtn = new QPushButton(QStringLiteral("Pause"), this);
    m_pauseBtn->setStyleSheet(btnStyle);
    m_pauseBtn->setVisible(false);
    connect(m_pauseBtn, &QPushButton::clicked, this, &MiniModeWindow::pauseClicked);
    btnRow->addWidget(m_pauseBtn);

    m_resumeBtn = new QPushButton(QStringLiteral("Resume"), this);
    m_resumeBtn->setStyleSheet(btnStyle);
    m_resumeBtn->setVisible(false);
    connect(m_resumeBtn, &QPushButton::clicked, this, &MiniModeWindow::resumeClicked);
    btnRow->addWidget(m_resumeBtn);

    btnRow->addStretch();
    root->addLayout(btnRow);
}

void MiniModeWindow::applyStyle()
{
    setStyleSheet(QStringLiteral(
        "MiniModeWindow { background: %1; border: 1px solid %2; }"
    ).arg(Theme::Panel.name()).arg(Theme::Border.name()));
}

void MiniModeWindow::updateDisplay(int remainingSeconds, int elapsedSeconds, int totalSeconds)
{
    m_circularTimer->setProgress(remainingSeconds, totalSeconds);
}

void MiniModeWindow::updateState(TimerState state, const QString& taskLabel)
{
    m_taskLabel->setText(taskLabel.isEmpty() ? QStringLiteral("No task") : taskLabel);

    switch (state) {
    case TimerState::Focusing:
    case TimerState::ShortBreak:
    case TimerState::LongBreak:
        m_pauseBtn->setVisible(true);
        m_resumeBtn->setVisible(false);
        break;
    case TimerState::Paused:
        m_pauseBtn->setVisible(false);
        m_resumeBtn->setVisible(true);
        break;
    default:
        m_pauseBtn->setVisible(false);
        m_resumeBtn->setVisible(false);
        break;
    }
}

} // namespace chronos
