#include "services/TimerService.h"
#include "app/AppContext.h"
#include "storage/StorageManager.h"
#include "vendor/chronos/services/TimerService.h"
#include "vendor/chronos/services/TaskService.h"
#include "vendor/chronos/services/StatisticsService.h"
#include "vendor/chronos/models/Statistics.h"

#include <QDateTime>

namespace arete::services {

TimerService::TimerService(app::AppContext* context, QObject* parent)
    : QObject(parent), m_context(context)
{
    auto* engine = m_context->chronosTimer();
    connect(engine, &chronos::TimerService::stateChanged, this,
            [this] { syncFromEngine(); });
    connect(engine, &chronos::TimerService::tick, this,
            [this](int remaining, int, int) {
                emit tick(remaining, elapsedSeconds());
            });
}

TimerMode TimerService::mode() const
{
    switch (m_context->chronosTimer()->currentSessionType()) {
    case chronos::SessionType::Focus: return TimerMode::Focus;
    case chronos::SessionType::ShortBreak: return TimerMode::ShortBreak;
    case chronos::SessionType::LongBreak: return TimerMode::LongBreak;
    default: return TimerMode::Idle;
    }
}

bool TimerService::isRunning() const
{
    const auto state = m_context->chronosTimer()->state();
    return state == chronos::TimerState::Focusing
        || state == chronos::TimerState::ShortBreak
        || state == chronos::TimerState::LongBreak;
}

int TimerService::remainingSeconds() const
{
    return m_context->chronosTimer()->remainingSeconds();
}

int TimerService::elapsedSeconds() const
{
    return m_context->chronosTimer()->elapsedSeconds();
}

int TimerService::totalSeconds() const
{
    return m_context->chronosTimer()->totalSeconds();
}

qint64 TimerService::attachedTaskId() const
{
    return m_context->chronosTimer()->currentTaskId().toLongLong();
}

QString TimerService::attachedTaskTitle() const
{
    const QString id = m_context->chronosTimer()->currentTaskId();
    if (id.isEmpty()) return {};
    return m_context->chronosTasks()->task(id).title;
}

int TimerService::focusMinutesToday() const
{
    return static_cast<int>(m_context->chronosStats()->compute().today.focusSeconds / 60);
}

int TimerService::focusStreak() const
{
    return m_context->chronosStats()->compute().currentStreakDays;
}

int TimerService::sessionsToday() const
{
    return m_context->chronosStats()->compute().today.sessionsCompleted;
}

int TimerService::weeklyFocusMinutes() const
{
    const QDate today = QDate::currentDate();
    return static_cast<int>(m_context->chronosStats()->computeForDateRange(today.addDays(-6), today).focusSeconds / 60);
}

void TimerService::startFocus(qint64 taskId, const QString& taskTitle)
{
    if (taskId > 0) {
        // Mirror the task into the Chronos task store so session history
        // can resolve its title.
        chronos::Task t = m_context->chronosTasks()->task(QString::number(taskId));
        if (t.id.isEmpty()) {
            m_context->chronosTasks()->addTask(taskTitle, QString(), 1);
            t = m_context->chronosTasks()->tasks().last();
            // Rebind to the generated id.
            chronos::Task renamed = t;
            renamed.id = QString::number(taskId);
            m_context->chronosTasks()->deleteTask(t.id);
            m_context->chronosTasks()->addTask(renamed.title, renamed.description, renamed.estimatedSessions);
        } else if (t.title != taskTitle) {
            m_context->chronosTasks()->editTask(t.id, taskTitle, t.description, t.estimatedSessions);
        }
        m_context->chronosTimer()->startFocusForTask(QString::number(taskId));
    } else {
        m_context->chronosTimer()->startFocus();
    }
}

void TimerService::startShortBreak()
{
    m_context->chronosTimer()->proceedToBreak(QString());
}

void TimerService::startLongBreak()
{
    // The engine decides break length from settings after a session;
    // a long break is started by switching the break type in settings.
    m_context->chronosTimer()->proceedToBreak(QString());
}

void TimerService::startCustomTimer(int minutes, const QString& label)
{
    m_context->chronosTimer()->startCustom(label, minutes);
    emit stateChanged();
}

void TimerService::pause()
{
    m_context->chronosTimer()->pause();
}

void TimerService::resume()
{
    m_context->chronosTimer()->resume();
}

void TimerService::stop()
{
    m_context->chronosTimer()->stop();
}

void TimerService::syncFromEngine()
{
    emit stateChanged();
}

} // namespace arete::services
