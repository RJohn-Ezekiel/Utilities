#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QSet>

namespace arete {
namespace app { class AppContext; }

namespace services {

// Watches the calendar, the task list and the habits and posts in-app
// notifications when something is due soon. Everything is opt-in: each
// category has its own setting in Settings, and if the user switches
// reminders off nothing is ever posted.
class ReminderService : public QObject
{
    Q_OBJECT

public:
    explicit ReminderService(app::AppContext* context, QObject* parent = nullptr);

    // Runs one sweep immediately (startup) and then on a short timer.
    void start();

private:
    void sweep();
    void remindEvents(const QDateTime& now);
    void remindTasks(const QDateTime& now);
    void remindHabits(const QDateTime& now);

    app::AppContext* m_context;
    QTimer m_timer;
    QSet<QString> m_notified;
    int m_lastHabitReminderDay = 0;
};

} // namespace services
} // namespace arete