#include "services/ReminderService.h"
#include "app/AppContext.h"
#include "data/EventRepository.h"
#include "data/TaskRepository.h"
#include "data/HabitRepository.h"
#include "models/CalendarEvent.h"
#include "models/Task.h"
#include "models/Habit.h"
#include "arete/notifications/NotificationCenter.h"
#include "arete/settings/SettingsManager.h"

#include <QDateTime>

namespace arete::services {

ReminderService::ReminderService(app::AppContext* context, QObject* parent)
    : QObject(parent), m_context(context)
{
    m_timer.setInterval(30 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &ReminderService::sweep);
}

void ReminderService::start()
{
    sweep();
    m_timer.start();
}

void ReminderService::sweep()
{
    if (!m_context->settings()->get(QStringLiteral("reminders/enabled"), true)) return;
    const QDateTime now = QDateTime::currentDateTime();
    remindEvents(now);
    remindTasks(now);
    remindHabits(now);
}

void ReminderService::remindEvents(const QDateTime& now)
{
    if (!m_context->settings()->get(QStringLiteral("reminders/events"), true)) return;
    const int lead = m_context->settings()->get(QStringLiteral("reminders/leadMinutes"), 15);
    const QDateTime horizon = now.addSecs(lead * 60);

    const auto events = m_context->events()->between(now, horizon);
    for (const models::CalendarEvent& e : events) {
        if (e.allDay) continue;
        const QString key = QStringLiteral("ev:%1").arg(e.id);
        if (m_notified.contains(key)) continue;
        m_notified.insert(key);
        arete::notifications::NotificationCenter::instance().showInfo(
            QStringLiteral("Upcoming event"),
            QStringLiteral("%1 at %2")
                .arg(e.title)
                .arg(e.start.toString(QStringLiteral("HH:mm"))),
            8000);
    }
}

void ReminderService::remindTasks(const QDateTime& now)
{
    if (!m_context->settings()->get(QStringLiteral("reminders/tasks"), true)) return;
    const int lead = m_context->settings()->get(QStringLiteral("reminders/leadMinutes"), 15);
    const QDateTime horizon = now.addSecs(lead * 60);

    const auto tasks = m_context->tasks()->active();
    for (const models::Task& t : tasks) {
        if (!t.dueAt.isValid()) continue;
        if (t.status == models::TaskStatus::Completed || t.status == models::TaskStatus::Archived) {
            continue;
        }
        if (t.dueAt > horizon) continue; // due too far in the future (or overdue: notify now)
        const QString key = QStringLiteral("tk:%1").arg(t.id);
        if (m_notified.contains(key)) continue;
        m_notified.insert(key);
        arete::notifications::NotificationCenter::instance().showInfo(
            QStringLiteral("Task due"),
            QStringLiteral("%1 at %2")
                .arg(t.title)
                .arg(t.dueAt.toString(QStringLiteral("HH:mm"))),
            8000);
    }
}

void ReminderService::remindHabits(const QDateTime& now)
{
    if (!m_context->settings()->get(QStringLiteral("reminders/habits"), true)) return;
    const int hour = m_context->settings()->get(QStringLiteral("reminders/habitHour"), 20);
    if (now.time().hour() != hour) return;
    if (now.date().dayOfYear() == m_lastHabitReminderDay) return;
    m_lastHabitReminderDay = now.date().dayOfYear();

    int remaining = 0;
    for (const models::Habit& h : m_context->habits()->all()) {
        if (!h.doneToday) ++remaining;
    }
    if (remaining == 0) return;
    arete::notifications::NotificationCenter::instance().showInfo(
        QStringLiteral("Habit check-in"),
        QStringLiteral("%1 habit%2 left for today")
            .arg(remaining)
            .arg(remaining == 1 ? QString() : QStringLiteral("s")),
        8000);
}

} // namespace arete::services