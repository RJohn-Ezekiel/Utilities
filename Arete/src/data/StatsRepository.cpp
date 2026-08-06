#include "data/StatsRepository.h"
#include "data/DatabaseManager.h"
#include "arete/logging/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>

namespace arete::data {

using arete::logging::Logger;

namespace {
QVector<int> fillPerDay(const QDate& start, const QDate& end,
                        const QMap<QDate, int>& minutes)
{
    QVector<int> out;
    out.reserve(start.daysTo(end) + 1);
    for (QDate d = start; d <= end; d = d.addDays(1)) {
        out.append(minutes.value(d, 0));
    }
    return out;
}
} // namespace

StatsRepository::StatsRepository(DatabaseManager* db, QObject* parent)
    : QObject(parent), m_db(db)
{
}

QVector<int> StatsRepository::focusMinutesPerDay(const QDate& start, const QDate& end) const
{
    QMap<QDate, int> minutes;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT started_at, actual_seconds FROM focus_sessions"
                             " WHERE started_at >= ? AND started_at < ? AND kind = 0"));
    q.addBindValue(start.startOfDay().toString(Qt::ISODate));
    q.addBindValue(end.addDays(1).startOfDay().toString(Qt::ISODate));
    if (!q.exec()) {
        Logger::instance().error(QStringLiteral("Focus stats failed: %1").arg(q.lastError().text()),
                                 QStringLiteral("database"));
        return QVector<int>(start.daysTo(end) + 1, 0);
    }
    while (q.next()) {
        const QDateTime dt = QDateTime::fromString(q.value(0).toString(), Qt::ISODate);
        const int seconds = q.value(1).toInt();
        if (!dt.isValid() || seconds <= 0) continue;
        minutes[dt.date()] = minutes.value(dt.date()) + qMax(1, qRound(seconds / 60.0));
    }
    return fillPerDay(start, end, minutes);
}

int StatsRepository::focusMinutesOn(const QDate& date) const
{
    const QVector<int> day = focusMinutesPerDay(date, date);
    return day.isEmpty() ? 0 : day.first();
}

int StatsRepository::sessionsOn(const QDate& date) const
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM focus_sessions"
                             " WHERE started_at >= ? AND started_at < ? AND kind = 0"));
    q.addBindValue(date.startOfDay().toString(Qt::ISODate));
    q.addBindValue(date.addDays(1).startOfDay().toString(Qt::ISODate));
    if (!q.exec() || !q.next()) return 0;
    return q.value(0).toInt();
}

QMap<QString, qint64> StatsRepository::focusMinutesByProject(const QDate& start, const QDate& end) const
{
    QMap<QString, qint64> result;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral(
        "SELECT COALESCE(p.name, ''), SUM(f.actual_seconds) FROM focus_sessions f"
        " LEFT JOIN tasks t ON t.id = f.task_id"
        " LEFT JOIN projects p ON p.id = t.project_id"
        " WHERE f.started_at >= ? AND f.started_at < ? AND f.kind = 0"
        " GROUP BY p.id"));
    q.addBindValue(start.startOfDay().toString(Qt::ISODate));
    q.addBindValue(end.addDays(1).startOfDay().toString(Qt::ISODate));
    if (!q.exec()) {
        Logger::instance().error(QStringLiteral("Project focus stats failed: %1").arg(q.lastError().text()),
                                 QStringLiteral("database"));
        return result;
    }
    while (q.next()) {
        result.insert(q.value(0).toString(), qRound(q.value(1).toLongLong() / 60.0));
    }
    return result;
}

int StatsRepository::activeDays(const QDate& start, const QDate& end) const
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT COUNT(DISTINCT substr(started_at, 1, 10)) FROM focus_sessions"
                             " WHERE started_at >= ? AND started_at < ? AND kind = 0"));
    q.addBindValue(start.startOfDay().toString(Qt::ISODate));
    q.addBindValue(end.addDays(1).startOfDay().toString(Qt::ISODate));
    if (!q.exec() || !q.next()) return 0;
    return q.value(0).toInt();
}

} // namespace arete::data