#include "MainWindow.h"
#include "Theme.h"
#include "services/TaskService.h"
#include "services/StatisticsService.h"
#include "services/ReminderScheduler.h"
#include "services/NotificationService.h"
#include "TaskListWidget.h"
#include "DashboardWidget.h"
#include "StatisticsWidget.h"
#include "HistoryWidget.h"
#include "SettingsWidget.h"
#include "MiniModeWindow.h"
#include "SessionCompleteDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QShortcut>
#include <QKeySequence>
#include <QCloseEvent>

namespace chronos {

MainWindow::MainWindow(TimerService* timerService,
                       TaskService* taskService,
                       StatisticsService* statisticsService,
                       ReminderScheduler* reminderScheduler,
                       NotificationService* notificationService,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_timerService(timerService)
    , m_taskService(taskService)
    , m_statisticsService(statisticsService)
    , m_reminderScheduler(reminderScheduler)
    , m_notificationService(notificationService)
{
    setWindowTitle(QStringLiteral("Chronos"));
    setMinimumSize(900, 650);

    // Restore window geometry from settings
    auto settings = m_timerService->currentSettings();
    if (settings.rememberWindowSize) {
        resize(settings.windowWidth, settings.windowHeight);
    } else {
        resize(1100, 750);
    }

    applyGlobalStyleSheet();
    setupUi();
    setupConnections();
    setupShortcuts();

    m_toolbar->updateForState(m_timerService->state());
    m_statusBar->updateForState(m_timerService->state());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveWindowGeometry();
    QMainWindow::closeEvent(event);
}

void MainWindow::saveWindowGeometry()
{
    auto settings = m_timerService->currentSettings();
    if (settings.rememberWindowSize) {
        settings.windowWidth = width();
        settings.windowHeight = height();
        // Save directly to storage to persist geometry
        auto storage = m_timerService->storage();
        auto loaded = storage->loadSettings();
        loaded.windowWidth = width();
        loaded.windowHeight = height();
        loaded.rememberWindowSize = settings.rememberWindowSize;
        storage->saveSettings(loaded);
    }
}

// ── UI Setup ────────────────────────────────────────────────────────────

void MainWindow::setupUi()
{
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_toolbar = new ToolbarWidget(this);
    rootLayout->addWidget(m_toolbar);

    auto* bodyLayout = new QHBoxLayout();
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    m_sidebar = new SidebarWidget(this);
    bodyLayout->addWidget(m_sidebar);

    m_centralStack = new QStackedWidget(this);
    bodyLayout->addWidget(m_centralStack, 1);

    rootLayout->addLayout(bodyLayout, 1);

    m_statusBar = new StatusBarWidget(this);
    rootLayout->addWidget(m_statusBar);

    buildDashboardPage();
    buildTasksPage();
    buildHistoryPage();
    buildStatisticsPage();
    buildSettingsPage();

    m_centralStack->setCurrentIndex(SidebarWidget::Dashboard);

    // Mini-mode window (hidden initially)
    m_miniMode = new MiniModeWindow(m_timerService, nullptr);
}

void MainWindow::buildDashboardPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QHBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(32);

    auto* timerContainer = new QWidget(page);
    auto* timerLayout = new QVBoxLayout(timerContainer);
    timerLayout->setAlignment(Qt::AlignCenter);

    m_circularTimer = new CircularTimerWidget(timerContainer);
    m_circularTimer->setFixedSize(300, 300);
    timerLayout->addWidget(m_circularTimer, 0, Qt::AlignCenter);
    timerLayout->addSpacing(16);

    auto* infoLabel = new QLabel(QStringLiteral("Press Start to begin"), timerContainer);
    infoLabel->setObjectName(QStringLiteral("timerInfo"));
    infoLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 13px;").arg(Theme::SecondaryText.name()));
    infoLabel->setAlignment(Qt::AlignCenter);
    timerLayout->addWidget(infoLabel, 0, Qt::AlignCenter);

    layout->addWidget(timerContainer, 1);

    m_dashboard = new DashboardWidget(m_statisticsService, m_timerService->storage(), page);
    layout->addWidget(m_dashboard, 1);

    m_centralStack->addWidget(page);
}

void MainWindow::buildTasksPage()
{
    m_taskList = new TaskListWidget(m_taskService, this);
    m_centralStack->addWidget(m_taskList);
}

void MainWindow::buildHistoryPage()
{
    m_history = new HistoryWidget(m_timerService->storage(), this);
    m_centralStack->addWidget(m_history);
}

void MainWindow::buildStatisticsPage()
{
    m_statistics = new StatisticsWidget(m_statisticsService, this);
    m_centralStack->addWidget(m_statistics);
}

void MainWindow::buildSettingsPage()
{
    m_settings = new SettingsWidget(m_timerService->storage(),
                                    m_timerService, m_reminderScheduler, this);
    m_centralStack->addWidget(m_settings);
}

// ── Signal Connections ──────────────────────────────────────────────────

void MainWindow::setupConnections()
{
    connect(m_toolbar, &ToolbarWidget::startClicked,
            m_timerService, &TimerService::startFocus);
    connect(m_toolbar, &ToolbarWidget::pauseClicked,
            m_timerService, &TimerService::pause);
    connect(m_toolbar, &ToolbarWidget::resumeClicked,
            m_timerService, &TimerService::resume);
    connect(m_toolbar, &ToolbarWidget::stopClicked,
            m_timerService, &TimerService::stop);
    connect(m_toolbar, &ToolbarWidget::skipBreakClicked,
            m_timerService, &TimerService::skipBreak);

    connect(m_timerService, &TimerService::tick,
            this, &MainWindow::onTimerTick);
    connect(m_timerService, &TimerService::stateChanged,
            this, &MainWindow::onTimerStateChanged);
    connect(m_timerService, &TimerService::focusSessionCompleted,
            this, &MainWindow::onFocusSessionCompleted);
    connect(m_timerService, &TimerService::breakCompleted,
            this, &MainWindow::onBreakCompleted);

    connect(m_sidebar, &SidebarWidget::pageSelected,
            this, &MainWindow::onSidebarPageSelected);

    connect(m_toolbar, &ToolbarWidget::miniModeClicked,
            this, &MainWindow::toggleMiniMode);

    // Settings button in toolbar → sidebar
    connect(m_toolbar, &ToolbarWidget::settingsClicked, this, [this]() {
        m_sidebar->setActivePage(SidebarWidget::Settings);
        m_centralStack->setCurrentIndex(SidebarWidget::Settings);
    });

    // Task → focus
    connect(m_taskList, &TaskListWidget::startFocusRequested, this, [this](const QString& taskId) {
        m_timerService->startFocusForTask(taskId);
        m_sidebar->setActivePage(SidebarWidget::Dashboard);
        m_centralStack->setCurrentIndex(SidebarWidget::Dashboard);
    });

    // Reminder lifecycle
    connect(m_timerService, &TimerService::stateChanged,
            this, [this](TimerState state, SessionType /*sessionType*/) {
        if (state == TimerState::Focusing) {
            m_reminderScheduler->startAll();
        } else if (state == TimerState::Idle) {
            m_reminderScheduler->stopAll();
        }
    });

    // Reminder → notification + stats
    connect(m_reminderScheduler, &ReminderScheduler::reminderTriggered,
            this, [this](HealthReminderType type) {
        QString title, message;
        switch (type) {
        case HealthReminderType::Water:
            title = QStringLiteral("Hydration");
            message = QStringLiteral("Time to drink water."); break;
        case HealthReminderType::Stand:
            title = QStringLiteral("Movement");
            message = QStringLiteral("Time to stand up."); break;
        case HealthReminderType::Stretch:
            title = QStringLiteral("Stretch");
            message = QStringLiteral("Time for a stretch break."); break;
        case HealthReminderType::Eye:
            title = QStringLiteral("Eye Rest");
            message = QStringLiteral("Look away for 20 seconds (20-20-20 rule)."); break;
        }
        m_notificationService->showReminder(title, message);
        m_statisticsService->recordHealthAck(type);
    });

    connect(m_notificationService, &NotificationService::inAppNotification,
            this, [this](const QString& msg) {
        m_statusBar->setNotification(msg, 8000);
    });

    // Settings changed
    connect(m_settings, &SettingsWidget::settingsChanged,
            this, &MainWindow::onSettingsChanged);

    // Reset everything
    connect(m_settings, &SettingsWidget::resetAllRequested, this, [this]() {
        m_timerService->stop();
        m_timerService->storage()->clearAll();
        m_timerService->reloadSettings();
        m_reminderScheduler->configure(m_timerService->currentSettings());
        m_sidebar->setActivePage(SidebarWidget::Dashboard);
        m_centralStack->setCurrentIndex(SidebarWidget::Dashboard);
        if (m_settings) m_settings->loadSettings();
        m_statusBar->setNotification(
            QStringLiteral("All data reset to defaults"), 5000);
    });

    // Mini mode
    connect(m_miniMode, &MiniModeWindow::pauseClicked,
            m_timerService, &TimerService::pause);
    connect(m_miniMode, &MiniModeWindow::resumeClicked,
            m_timerService, &TimerService::resume);
    connect(m_miniMode, &MiniModeWindow::restoreClicked, this, [this]() {
        m_miniMode->hide();
        showNormal();
        raise();
        activateWindow();
    });

    // Keep mini mode in sync with main timer
    connect(m_timerService, &TimerService::tick, this, [this](int remaining, int elapsed, int total) {
        m_miniMode->updateDisplay(remaining, elapsed, total);
    });
    connect(m_timerService, &TimerService::stateChanged, this, [this](TimerState state, SessionType) {
        QString taskLabel = m_timerService->currentTaskId().isEmpty()
            ? QString() : m_taskService->task(m_timerService->currentTaskId()).title;
        m_miniMode->updateState(state, taskLabel);
    });
}

void MainWindow::setupShortcuts()
{
    auto* space = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(space, &QShortcut::activated, this, [this]() {
        switch (m_timerService->state()) {
        case TimerState::Focusing:
        case TimerState::ShortBreak:
        case TimerState::LongBreak:
            m_timerService->pause(); break;
        case TimerState::Paused:
            m_timerService->resume(); break;
        default: break;
        }
    });

    auto* ctrlN = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_N), this);
    connect(ctrlN, &QShortcut::activated, this, [this]() {
        m_sidebar->setActivePage(SidebarWidget::Tasks);
        m_centralStack->setCurrentIndex(SidebarWidget::Tasks);
        m_statusBar->setNotification(QStringLiteral("Ctrl+N: New Task"), 3000);
    });

    auto* ctrlS = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
    connect(ctrlS, &QShortcut::activated, m_timerService, &TimerService::startFocus);

    auto* ctrlR = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), this);
    connect(ctrlR, &QShortcut::activated, m_timerService, &TimerService::startFocus);

    auto* ctrlK = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), this);
    connect(ctrlK, &QShortcut::activated, m_timerService, &TimerService::skipBreak);

    auto* ctrlM = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_M), this);
    connect(ctrlM, &QShortcut::activated, this, &MainWindow::toggleMiniMode);

    auto* f1 = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(f1, &QShortcut::activated, this, [this]() {
        m_statusBar->setNotification(
            QStringLiteral("Space:Pause  Ctrl+N:Task  Ctrl+S:Start  Ctrl+R:Restart"
                           "  Ctrl+K:Skip  Ctrl+M:Mini  Ctrl+Q:Quit"), 8000);
    });

    auto* ctrlQ = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
    connect(ctrlQ, &QShortcut::activated, qApp, &QApplication::quit);
}

void MainWindow::applyGlobalStyleSheet()
{
    qApp->setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  font-family: \"JetBrains Mono\", \"Cascadia Mono\", \"Fira Code\","
        "               \"Noto Sans Mono\", monospace;"
        "}"
        "QToolTip {"
        "  background-color: %3;"
        "  color: %2;"
        "  border: 1px solid %4;"
        "  padding: 4px 8px;"
        "  font-size: 12px;"
        "}"
    )
        .arg(Theme::Background.name())
        .arg(Theme::PrimaryText.name())
        .arg(Theme::Panel.name())
        .arg(Theme::Border.name()));
}

// ── Slots ───────────────────────────────────────────────────────────────

void MainWindow::onTimerTick(int remainingSeconds, int, int totalSeconds)
{
    m_circularTimer->setProgress(remainingSeconds, totalSeconds);

    int min = remainingSeconds / 60;
    int sec = remainingSeconds % 60;
    m_statusBar->setStateText(
        QStringLiteral("%1 \u2014 %2:%3 remaining")
            .arg(m_statusBar->property("statePrefix").toString())
            .arg(min, 2, 10, QChar('0'))
            .arg(sec, 2, 10, QChar('0')));
}

void MainWindow::onTimerStateChanged(TimerState state, SessionType)
{
    m_toolbar->updateForState(state);
    m_statusBar->updateForState(state);

    QString taskLabel;
    if (!m_timerService->currentTaskId().isEmpty()) {
        taskLabel = m_taskService->task(m_timerService->currentTaskId()).title;
    }

    switch (state) {
    case TimerState::Focusing: {
        int total = m_timerService->totalSeconds();
        int m = total / 60, s = total % 60;
        m_circularTimer->setSessionLabel(
            QStringLiteral("Focus Session \u00B7 %1:%2")
                .arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')));
        m_circularTimer->setTaskLabel(taskLabel);
        m_statusBar->setProperty("statePrefix", QStringLiteral("Focusing"));
        break;
    }
    case TimerState::ShortBreak:
        m_circularTimer->setSessionLabel(QStringLiteral("Short Break"));
        m_statusBar->setProperty("statePrefix", QStringLiteral("Short Break"));
        break;
    case TimerState::LongBreak:
        m_circularTimer->setSessionLabel(QStringLiteral("Long Break"));
        m_statusBar->setProperty("statePrefix", QStringLiteral("Long Break"));
        break;
    case TimerState::Paused:
        m_statusBar->setProperty("statePrefix", QStringLiteral("Paused"));
        break;
    case TimerState::Idle:
        m_circularTimer->clear();
        m_statusBar->setProperty("statePrefix", QStringLiteral("Idle"));
        m_dashboard->refresh();
        m_statistics->refresh();
        break;
    }

    m_miniMode->updateState(state, taskLabel);
}

void MainWindow::onFocusSessionCompleted(const QString& sessionId,
                                         const QString& taskId,
                                         int durationSeconds)
{
    int durationMin = durationSeconds / 60;

    QString taskTitle;
    if (!taskId.isEmpty()) {
        taskTitle = m_taskService->task(taskId).title;
    }

    SessionCompleteDialog dialog(durationMin, taskTitle, this);
    if (dialog.exec() == QDialog::Accepted) {
        auto result = dialog.result();
        if (result.markTaskCompleted && !taskId.isEmpty()) {
            m_taskService->completeTask(taskId);
        }
        switch (result.action) {
        case SessionCompleteDialog::Result::ProceedToBreak:
            m_timerService->proceedToBreak(result.note); break;
        case SessionCompleteDialog::Result::SkipBreak:
            m_timerService->skipAfterSession(result.note); break;
        }
    } else {
        m_timerService->proceedToBreak(QString());
    }

    m_dashboard->refresh();
    m_statistics->refresh();
    if (m_history) m_history->refresh();
    m_statusBar->setNotification(
        QStringLiteral("Focus session completed \u2014 %1 min").arg(durationMin), 5000);
}

void MainWindow::onBreakCompleted()
{
    m_statusBar->setNotification(
        QStringLiteral("Break finished \u2014 ready to focus"), 5000);
    m_dashboard->refresh();
    m_statistics->refresh();
    if (m_history) m_history->refresh();
    m_notificationService->showNotification(
        QStringLiteral("Break Over"), QStringLiteral("Break finished \u2014 ready to focus"));
}

void MainWindow::onSidebarPageSelected(SidebarWidget::Page page)
{
    m_centralStack->setCurrentIndex(static_cast<int>(page));
    switch (page) {
    case SidebarWidget::History:
        if (m_history) m_history->refresh(); break;
    case SidebarWidget::Statistics:
        if (m_statistics) m_statistics->refresh(); break;
    case SidebarWidget::Settings:
        if (m_settings) m_settings->loadSettings(); break;
    default: break;
    }
}

void MainWindow::onSettingsChanged()
{
    auto settings = m_timerService->currentSettings();

    if (settings.alwaysOnTop) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    }
    show(); // reapply flags

    m_statusBar->setNotification(QStringLiteral("Settings saved"), 3000);
}

void MainWindow::toggleMiniMode()
{
    if (m_miniMode->isVisible()) {
        m_miniMode->hide();
    } else {
        m_miniMode->show();
        // Sync current state
        auto state = m_timerService->state();
        QString taskLabel;
        if (!m_timerService->currentTaskId().isEmpty()) {
            taskLabel = m_taskService->task(m_timerService->currentTaskId()).title;
        }
        m_miniMode->updateState(state, taskLabel);
        m_miniMode->updateDisplay(m_timerService->remainingSeconds(),
                                  m_timerService->elapsedSeconds(),
                                  m_timerService->totalSeconds());
    }
}

} // namespace chronos
