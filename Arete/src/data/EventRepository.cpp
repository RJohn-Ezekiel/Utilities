#include "data/EventRepository.h"
#include "data/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace arete::data {

namespace {
models::CalendarEvent readEvent(const QSqlQuery& q)
{
    models::CalendarEvent e;
    e.id = q.value(QStringLiteral("id")).toLongLong();
    e.title = q.value(QStringLiteral("title")).toString();
    e.notes = q.value(QStringLiteral("notes")).toString();
    e.start = QDateTime::fromString(q.value(QStringLiteral("start_at")).toString(), Qt::ISODate);
    e.end = QDateTime::fromString(q.value(QStringLiteral("end_at")).toString(), Qt::ISODate);
    e.recurrence = static_cast<models::Recurrence>(q.value(QStringLiteral("recurrence")).toInt());
    e.allDay = q.value(QStringLiteral("all_day")).toBool();
    e.taskId = q.value(QStringLiteral("task_id")).toLongLong();
    e.createdAt = QDateTime::fromString(q.value(QStringLiteral("created_at")).toString(), Qt::ISODate);
    return e;
}

QDate nextOccurrence(models::Recurrence r, const QDate& from)
{
    switch (r) {
    case models::Recurrence::Daily: return from.addDays(1);
    case models::Recurrence::Weekly: return from.addDays(7);
    case models::Recurrence::Monthly: return from.addMonths(1);
    case models::Recurrence::Yearly: return from.addYears(1);
    default: return QDate();
    }
}
} // namespace

EventRepository::EventRepository(DatabaseManager* db, QObject* parent)
    : QObject(parent), m_db(db)
{
}

QVector<models::CalendarEvent> EventRepository::all() const
{
    QVector<models::CalendarEvent> result;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT * FROM events ORDER BY start_at"));
    if (!q.exec()) return result;
    while (q.next()) result.append(readEvent(q));
    return result;
}

QVector<models::CalendarEvent> EventRepository::between(const QDateTime& from, const QDateTime& to) const
{
    QVector<models::CalendarEvent> result;
    const QVector<models::CalendarEvent> allEvents = all();
    for (const models::CalendarEvent& e : allEvents) {
        if (!e.start.isValid()) continue;

        // Recurring events are expanded within the window.
        if (e.recurrence != models::Recurrence::None) {
            QDate occurrence = e.start.date();
            while (occurrence.isValid() && occurrence < from.date()) {
                occurrence = nextOccurrence(e.recurrence, occurrence);
            }
            while (occurrence.isValid() && occurrence <= to.date()) {
                const qint64 dayOffset = e.start.date().daysTo(occurrence);
                models::CalendarEvent copy = e;
                copy.id = -1; // synthetic occurrence
                copy.start = e.start.addDays(dayOffset);
                copy.end = e.end.isValid() ? e.end.addDays(dayOffset) : copy.start;
                if (copy.overlaps(from, to)) result.append(copy);
                const QDate next = nextOccurrence(e.recurrence, occurrence);
                if (!next.isValid() || next <= occurrence) break; // safety
                occurrence = next;
            }
        } else if (e.overlaps(from, to)) {
            result.append(e);
        }
    }
    std::sort(result.begin(), result.end(),
              [](const models::CalendarEvent& a, const models::CalendarEvent& b) {
                  return a.start < b.start;
              });
    return result;
}

models::CalendarEvent EventRepository::byId(qint64 id) const
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT * FROM events WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next()) return {};
    return readEvent(q);
}

qint64 EventRepository::insert(const models::CalendarEvent& event)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("INSERT INTO events (title, notes, start_at, end_at, recurrence, "
                             "all_day, task_id, created_at) VALUES (:title, :notes, :start_at, "
                             ":end_at, :recurrence, :all_day, :task_id, :created_at)"));
    q.bindValue(QStringLiteral(":title"), event.title);
    q.bindValue(QStringLiteral(":notes"), event.notes);
    q.bindValue(QStringLiteral(":start_at"), event.start.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":end_at"), event.end.isValid() ? event.end.toString(Qt::ISODate) : QVariant());
    q.bindValue(QStringLiteral(":recurrence"), static_cast<int>(event.recurrence));
    q.bindValue(QStringLiteral(":all_day"), event.allDay ? 1 : 0);
    q.bindValue(QStringLiteral(":task_id"), event.taskId);
    q.bindValue(QStringLiteral(":created_at"),
                event.createdAt.isValid() ? event.createdAt.toString(Qt::ISODate)
                                          : QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) return -1;
    const qint64 id = q.lastInsertId().toLongLong();
    emit changed();
    return id;
}

bool EventRepository::update(const models::CalendarEvent& event)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("UPDATE events SET title=:title, notes=:notes, start_at=:start_at, "
                             "end_at=:end_at, recurrence=:recurrence, all_day=:all_day, task_id=:task_id "
                             "WHERE id=:id"));
    q.bindValue(QStringLiteral(":title"), event.title);
    q.bindValue(QStringLiteral(":notes"), event.notes);
    q.bindValue(QStringLiteral(":start_at"), event.start.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":end_at"), event.end.isValid() ? event.end.toString(Qt::ISODate) : QVariant());
    q.bindValue(QStringLiteral(":recurrence"), static_cast<int>(event.recurrence));
    q.bindValue(QStringLiteral(":all_day"), event.allDay ? 1 : 0);
    q.bindValue(QStringLiteral(":task_id"), event.taskId);
    q.bindValue(QStringLiteral(":id"), event.id);
    const bool ok = q.exec();
    if (ok) emit changed();
    return ok;
}

bool EventRepository::remove(qint64 id)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("DELETE FROM events WHERE id=:id"));
    q.bindValue(QStringLiteral(":id"), id);
    const bool ok = q.exec();
    if (ok) emit changed();
    return ok;
}

} // namespace arete::data
