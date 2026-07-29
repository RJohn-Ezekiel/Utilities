#pragma once

#include <QMainWindow>
#include <QStackedWidget>

#include "core/TimerEngine.h"
#include "services/TimerService.h"

#include "ToolbarWidget.h"
#include "SidebarWidget.h"
#include "StatusBarWidget.h"
#include "CircularTimerWidget.h"

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace chronos {

class TaskService;
class TaskListWidget;
class ReminderScheduler;
class NotificationService;
class StatisticsService;
class DashboardWidget;
class StatisticsWidget;
class HistoryWidget;
class SettingsWidget;
class MiniModeWindow;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(TimerService* timerService,
                        TaskService* taskService,
                        StatisticsService* statisticsService,
                        ReminderScheduler* reminderScheduler,
                        NotificationService* notificationService,
                        QWidget* parent = nullptr);

    void saveWindowGeometry();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onTimerTick(int remainingSeconds, int elapsedSeconds, int totalSeconds);
    void onTimerStateChanged(TimerState state, SessionType sessionType);
    void onFocusSessionCompleted(const QString& sessionId,
                                 const QString& taskId,
                                 int durationSeconds);
    void onBreakCompleted();
    void onSidebarPageSelected(SidebarWidget::Page page);
    void onSettingsChanged();

private:
    void setupUi();
    void setupConnections();
    void setupShortcuts();
    void applyGlobalStyleSheet();
    void buildDashboardPage();
    void buildTasksPage();
    void buildHistoryPage();
    void buildStatisticsPage();
    void buildSettingsPage();
    void toggleMiniMode();

    TimerService* m_timerService = nullptr;
    TaskService* m_taskService = nullptr;
    StatisticsService* m_statisticsService = nullptr;
    ReminderScheduler* m_reminderScheduler = nullptr;
    NotificationService* m_notificationService = nullptr;

    ToolbarWidget* m_toolbar = nullptr;
    SidebarWidget* m_sidebar = nullptr;
    StatusBarWidget* m_statusBar = nullptr;
    QStackedWidget* m_centralStack = nullptr;

    CircularTimerWidget* m_circularTimer = nullptr;
    DashboardWidget* m_dashboard = nullptr;
    TaskListWidget* m_taskList = nullptr;
    HistoryWidget* m_history = nullptr;
    StatisticsWidget* m_statistics = nullptr;
    SettingsWidget* m_settings = nullptr;
    MiniModeWindow* m_miniMode = nullptr;
};

} // namespace chronos
