#include "data/HabitRepository.h"
#include "data/DatabaseManager.h"
#include "arete/logging/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDate>

namespace arete::data {

using arete::logging::Logger;

namespace {
QString dateKey(const QDate& date)
{
    return date.toString(Qt::ISODate);
}
} // namespace

HabitRepository::HabitRepository(DatabaseManager* db, QObject* parent)
    : QObject(parent), m_db(db)
{
}

QVector<models::Habit> HabitRepository::all() const
{
    QVector<models::Habit> habits;
    QSqlQuery q(m_db->connection());
    if (!q.exec(QStringLiteral("SELECT id, name, target_per_week, created_at FROM habits ORDER BY name COLLATE NOCASE"))) {
        Logger::instance().error(QStringLiteral("Habit query failed: %1").arg(q.lastError().text()),
                                 QStringLiteral("database"));
        return habits;
    }

    const QDate today = QDate::currentDate();
    const QDate weekStart = today.addDays(-(today.dayOfWeek() - 1)); // Monday
    while (q.next()) {
        models::Habit habit;
        habit.id = q.value(0).toLongLong();
        habit.name = q.value(1).toString();
        habit.targetPerWeek = q.value(2).toInt();
        habit.createdAt = QDateTime::fromString(q.value(3).toString(), Qt::ISODate);

        const QSet<QDate> dates = doneDates(habit.id);
        habit.doneToday = dates.contains(today);
        int done = 0;
        for (const QDate& d : dates) {
            if (d >= weekStart && d <= today) ++done;
        }
        habit.doneThisWeek = done;

        const Streaks streaks = computeStreaks(dates);
        habit.currentStreak = streaks.current;
        habit.longestStreak = streaks.longest;
        habits.append(habit);
    }
    return habits;
}

models::Habit HabitRepository::byId(qint64 id) const
{
    for (const models::Habit& h : all()) {
        if (h.id == id) return h;
    }
    return {};
}

qint64 HabitRepository::insert(const QString& name, int targetPerWeek)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("INSERT INTO habits (name, target_per_week, created_at) VALUES (?, ?, ?)"));
    q.addBindValue(name.trimmed());
    q.addBindValue(targetPerWeek);
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) {
        Logger::instance().error(QStringLiteral("Habit insert failed: %1").arg(q.lastError().text()),
                                 QStringLiteral("database"));
        return -1;
    }
    emit changed();
    return q.lastInsertId().toLongLong();
}

bool HabitRepository::update(qint64 id, const QString& name, int targetPerWeek)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("UPDATE habits SET name = ?, target_per_week = ? WHERE id = ?"));
    q.addBindValue(name.trimmed());
    q.addBindValue(targetPerWeek);
    q.addBindValue(id);
    if (!q.exec()) {
        Logger::instance().error(QStringLiteral("Habit update failed: %1").arg(q.lastError().text()),
                                 QStringLiteral("database"));
        return false;
    }
    emit changed();
    return true;
}

bool HabitRepository::remove(qint64 id)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("DELETE FROM habits WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        Logger::instance().error(QStringLiteral("Habit delete failed: %1").arg(q.lastError().text()),
                                 QStringLiteral("database"));
        return false;
    }
    QSqlQuery q2(m_db->connection());
    q2.prepare(QStringLiteral("DELETE FROM habit_logs WHERE habit_id = ?"));
    q2.addBindValue(id);
    q2.exec();
    emit changed();
    return true;
}

bool HabitRepository::toggle(qint64 habitId, const QDate& date)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT 1 FROM habit_logs WHERE habit_id = ? AND date = ?"));
    q.addBindValue(habitId);
    q.addBindValue(dateKey(date));
    if (!q.exec() || !q.next()) {
        // Not logged yet -> add.
        QSqlQuery ins(m_db->connection());
        ins.prepare(QStringLiteral("INSERT INTO habit_logs (habit_id, date) VALUES (?, ?)"));
        ins.addBindValue(habitId);
        ins.addBindValue(dateKey(date));
        if (!ins.exec()) {
            Logger::instance().error(QStringLiteral("Habit log insert failed: %1").arg(ins.lastError().text()),
                                     QStringLiteral("database"));
            return false;
        }
        emit changed();
        return true;
    }
    // Already logged -> remove.
    QSqlQuery del(m_db->connection());
    del.prepare(QStringLiteral("DELETE FROM habit_logs WHERE habit_id = ? AND date = ?"));
    del.addBindValue(habitId);
    del.addBindValue(dateKey(date));
    if (!del.exec()) {
        Logger::instance().error(QStringLiteral("Habit log delete failed: %1").arg(del.lastError().text()),
                                 QStringLiteral("database"));
        return false;
    }
    emit changed();
    return false;
}

bool HabitRepository::isDone(qint64 habitId, const QDate& date) const
{
    return doneDates(habitId).contains(date);
}

QSet<QDate> HabitRepository::doneDates(qint64 habitId) const
{
    QSet<QDate> dates;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT date FROM habit_logs WHERE habit_id = ?"));
    q.addBindValue(habitId);
    if (!q.exec()) {
        Logger::instance().error(QStringLiteral("Habit log query failed: %1").arg(q.lastError().text()),
                                 QStringLiteral("database"));
        return dates;
    }
    while (q.next()) {
        const QDate d = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        if (d.isValid()) dates.insert(d);
    }
    return dates;
}

int HabitRepository::completedInRange(qint64 habitId, const QDate& start, const QDate& end) const
{
    int count = 0;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM habit_logs WHERE habit_id = ? AND date BETWEEN ? AND ?"));
    q.addBindValue(habitId);
    q.addBindValue(start.toString(Qt::ISODate));
    q.addBindValue(end.toString(Qt::ISODate));
    if (!q.exec()) {
        Logger::instance().error(QStringLiteral("Habit range query failed: %1").arg(q.lastError().text()),
                                 QStringLiteral("database"));
        return 0;
    }
    if (q.next()) count = q.value(0).toInt();
    return count;
}

HabitRepository::Streaks HabitRepository::computeStreaks(const QSet<QDate>& dates) const
{
    Streaks result;
    if (dates.isEmpty()) return result;

    QList<QDate> sorted = dates.values();
    std::sort(sorted.begin(), sorted.end());

    // Longest run over all dates.
    int run = 1;
    result.longest = 1;
    for (int i = 1; i < sorted.size(); ++i) {
        if (sorted.at(i) == sorted.at(i - 1).addDays(1)) {
            ++run;
            result.longest = qMax(result.longest, run);
        } else {
            run = 1;
        }
    }

    // Current streak: consecutive days ending today (or yesterday if today
    // is not done yet — the streak is still alive until the day ends).
    const QDate today = QDate::currentDate();
    int current = 0;
    QDate cursor = today;
    if (!sorted.contains(today)) cursor = today.addDays(-1);
    while (sorted.contains(cursor)) {
        ++current;
        cursor = cursor.addDays(-1);
    }
    result.current = current;
    return result;
}

} // namespace arete::data