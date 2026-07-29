#include "StatisticsService.h"
#include "services/TaskService.h"

#include <algorithm>

namespace chronos {

StatisticsService::StatisticsService(StorageManager* storage,
                                     TaskService* taskService,
                                     QObject* parent)
    : QObject(parent)
    , m_storage(storage)
    , m_taskService(taskService)
{
    m_cache = m_storage->loadStatistics();
}

Statistics StatisticsService::compute()
{
    auto sessions = m_storage->loadSessions();

    m_cache.today = computeForDateRange(QDate::currentDate(), QDate::currentDate());
    m_cache.currentStreakDays = computeStreak();
    m_cache.averageDailyFocusSeconds = 0.0;
    m_cache.longestSessionSeconds = 0;

    // Aggregate daily history and compute averages
    QMap<QDate, DailyStats> dailyMap;
    qint64 totalFocusSeconds = 0;
    int daysWithData = 0;

    for (const auto& session : sessions) {
        QDate date = session.startTime.date();
        if (!date.isValid()) continue;

        auto& ds = dailyMap[date];
        if (session.type == SessionType::Focus && session.completed) {
            ds.focusSeconds += session.durationSeconds;
            ds.sessionsCompleted++;
            if (session.durationSeconds > m_cache.longestSessionSeconds) {
                m_cache.longestSessionSeconds = session.durationSeconds;
            }
        } else if (session.completed) {
            ds.breakSeconds += session.durationSeconds;
        }
    }

    // Count completed tasks per day
    for (const auto& task : m_taskService->tasks()) {
        if (task.completed && task.completedAt.isValid()) {
            QDate date = task.completedAt.date();
            dailyMap[date].tasksCompleted++;
        }
    }

    for (auto it = dailyMap.constBegin(); it != dailyMap.constEnd(); ++it) {
        totalFocusSeconds += it.value().focusSeconds;
        if (it.value().focusSeconds > 0) {
            daysWithData++;
        }
    }

    if (daysWithData > 0) {
        m_cache.averageDailyFocusSeconds = static_cast<double>(totalFocusSeconds)
                                          / static_cast<double>(daysWithData);
    }

    // Update today's stats with health ack counts from saved data
    QDate today = QDate::currentDate();
    if (dailyMap.contains(today)) {
        m_cache.today.waterAcks = dailyMap[today].waterAcks;
        m_cache.today.standAcks = dailyMap[today].standAcks;
        m_cache.today.stretchAcks = dailyMap[today].stretchAcks;
        m_cache.today.eyeAcks = dailyMap[today].eyeAcks;
    }

    m_cache.dailyHistory = dailyMap;
    m_storage->saveStatistics(m_cache);
    emit statisticsUpdated();
    return m_cache;
}

DailyStats StatisticsService::computeForDateRange(const QDate& start, const QDate& end)
{
    DailyStats result;
    auto sessions = m_storage->loadSessions();

    for (const auto& session : sessions) {
        QDate date = session.startTime.date();
        if (date >= start && date <= end && session.completed) {
            if (session.type == SessionType::Focus) {
                result.focusSeconds += session.durationSeconds;
                result.sessionsCompleted++;
            } else {
                result.breakSeconds += session.durationSeconds;
            }
        }
    }

    for (const auto& task : m_taskService->tasks()) {
        if (task.completed && task.completedAt.isValid()) {
            QDate date = task.completedAt.date();
            if (date >= start && date <= end) {
                result.tasksCompleted++;
            }
        }
    }

    return result;
}

int StatisticsService::computeStreak()
{
    auto sessions = m_storage->loadSessions();
    QSet<QDate> focusDates;

    for (const auto& session : sessions) {
        if (session.type == SessionType::Focus && session.completed) {
            focusDates.insert(session.startTime.date());
        }
    }

    QDate today = QDate::currentDate();
    int streak = 0;
    QDate check = today;

    // Check if today has a session; if not, start from yesterday
    if (!focusDates.contains(today)) {
        check = today.addDays(-1);
    }

    while (focusDates.contains(check)) {
        streak++;
        check = check.addDays(-1);
    }

    return streak;
}

void StatisticsService::recordHealthAck(HealthReminderType type)
{
    QDate today = QDate::currentDate();

    switch (type) {
    case HealthReminderType::Water:  m_cache.today.waterAcks++;   break;
    case HealthReminderType::Stand:  m_cache.today.standAcks++;   break;
    case HealthReminderType::Stretch: m_cache.today.stretchAcks++; break;
    case HealthReminderType::Eye:    m_cache.today.eyeAcks++;     break;
    }

    m_cache.dailyHistory[today] = m_cache.today;
    m_storage->saveStatistics(m_cache);
    emit statisticsUpdated();
}

} // namespace chronos
