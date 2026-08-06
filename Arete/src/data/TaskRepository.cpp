#include "data/TaskRepository.h"
#include "data/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDateTime>

namespace arete::data {

namespace {
constexpr int kActive = static_cast<int>(models::TaskStatus::Active);
constexpr int kCompleted = static_cast<int>(models::TaskStatus::Completed);

void bindTask(QSqlQuery& q, const models::Task& t)
{
    q.bindValue(QStringLiteral(":title"), t.title);
    q.bindValue(QStringLiteral(":notes"), t.notes);
    q.bindValue(QStringLiteral(":project_id"), t.projectId);
    q.bindValue(QStringLiteral(":priority"), static_cast<int>(t.priority));
    q.bindValue(QStringLiteral(":status"), static_cast<int>(t.status));
    q.bindValue(QStringLiteral(":due_at"), t.dueAt.isValid() ? t.dueAt.toString(Qt::ISODate) : QVariant());
    q.bindValue(QStringLiteral(":recurring"), t.recurring);
    q.bindValue(QStringLiteral(":checklist"), t.checklist.join(QLatin1Char('\n')));
    q.bindValue(QStringLiteral(":checked_items"), t.checkedItems.join(QLatin1Char('\n')));
    q.bindValue(QStringLiteral(":sort_order"), t.sortOrder);
    q.bindValue(QStringLiteral(":created_at"), t.createdAt.isValid() ? t.createdAt.toString(Qt::ISODate) : QVariant());
    q.bindValue(QStringLiteral(":completed_at"), t.completedAt.isValid() ? t.completedAt.toString(Qt::ISODate) : QVariant());
}

models::Task readTask(const QSqlQuery& q)
{
    models::Task t;
    t.id = q.value(QStringLiteral("id")).toLongLong();
    t.title = q.value(QStringLiteral("title")).toString();
    t.notes = q.value(QStringLiteral("notes")).toString();
    t.projectId = q.value(QStringLiteral("project_id")).toLongLong();
    t.priority = static_cast<models::Priority>(q.value(QStringLiteral("priority")).toInt());
    t.status = static_cast<models::TaskStatus>(q.value(QStringLiteral("status")).toInt());
    t.dueAt = QDateTime::fromString(q.value(QStringLiteral("due_at")).toString(), Qt::ISODate);
    t.recurring = q.value(QStringLiteral("recurring")).toString();
    t.checklist = q.value(QStringLiteral("checklist")).toString().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    t.checkedItems = q.value(QStringLiteral("checked_items")).toString().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    t.sortOrder = q.value(QStringLiteral("sort_order")).toLongLong();
    t.createdAt = QDateTime::fromString(q.value(QStringLiteral("created_at")).toString(), Qt::ISODate);
    t.completedAt = QDateTime::fromString(q.value(QStringLiteral("completed_at")).toString(), Qt::ISODate);
    return t;
}
} // namespace

TaskRepository::TaskRepository(DatabaseManager* db, QObject* parent)
    : QObject(parent), m_db(db)
{
}

QVector<models::Task> TaskRepository::all() const
{
    QVector<models::Task> result;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT * FROM tasks ORDER BY sort_order, id"));
    if (!q.exec()) return result;
    while (q.next()) result.append(readTask(q));
    return result;
}

QVector<models::Task> TaskRepository::active() const
{
    QVector<models::Task> result;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT * FROM tasks WHERE status = :active "
                             "ORDER BY (due_at IS NULL), due_at, sort_order, id"));
    q.bindValue(QStringLiteral(":active"), kActive);
    if (!q.exec()) return result;
    while (q.next()) result.append(readTask(q));
    return result;
}

QVector<models::Task> TaskRepository::forProject(qint64 projectId) const
{
    QVector<models::Task> result;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT * FROM tasks WHERE project_id = :project ORDER BY sort_order, id"));
    q.bindValue(QStringLiteral(":project"), projectId);
    if (!q.exec()) return result;
    while (q.next()) result.append(readTask(q));
    return result;
}

models::Task TaskRepository::byId(qint64 id) const
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT * FROM tasks WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next()) return {};
    return readTask(q);
}

qint64 TaskRepository::insert(const models::Task& task)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("INSERT INTO tasks (title, notes, project_id, priority, status, due_at, "
                             "recurring, checklist, checked_items, sort_order, created_at, completed_at) "
                             "VALUES (:title, :notes, :project_id, :priority, :status, :due_at, "
                             ":recurring, :checklist, :checked_items, :sort_order, :created_at, :completed_at)"));
    bindTask(q, task);
    if (!q.exec()) return -1;
    const qint64 id = q.lastInsertId().toLongLong();
    emit changed();
    return id;
}

bool TaskRepository::update(const models::Task& task)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("UPDATE tasks SET title=:title, notes=:notes, project_id=:project_id, "
                             "priority=:priority, status=:status, due_at=:due_at, recurring=:recurring, "
                             "checklist=:checklist, checked_items=:checked_items, sort_order=:sort_order, "
                             "created_at=:created_at, completed_at=:completed_at WHERE id=:id"));
    bindTask(q, task);
    q.bindValue(QStringLiteral(":id"), task.id);
    const bool ok = q.exec();
    if (ok) emit changed();
    return ok;
}

bool TaskRepository::remove(qint64 id)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("DELETE FROM tasks WHERE id=:id"));
    q.bindValue(QStringLiteral(":id"), id);
    const bool ok = q.exec();
    if (ok) emit changed();
    return ok;
}

bool TaskRepository::setStatus(qint64 id, models::TaskStatus status, bool completed,
                               const QDateTime& completedAt)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("UPDATE tasks SET status=:status, completed_at=:completed_at WHERE id=:id"));
    q.bindValue(QStringLiteral(":status"), static_cast<int>(status));
    q.bindValue(QStringLiteral(":completed_at"),
                (completed && completedAt.isValid()) ? completedAt.toString(Qt::ISODate) : QVariant());
    q.bindValue(QStringLiteral(":id"), id);
    const bool ok = q.exec();
    if (ok) emit changed();
    return ok;
}

bool TaskRepository::setProject(qint64 id, qint64 projectId)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("UPDATE tasks SET project_id=:project WHERE id=:id"));
    q.bindValue(QStringLiteral(":project"), projectId);
    q.bindValue(QStringLiteral(":id"), id);
    const bool ok = q.exec();
    if (ok) emit changed();
    return ok;
}

bool TaskRepository::setSortOrder(qint64 id, qint64 order)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("UPDATE tasks SET sort_order=:order WHERE id=:id"));
    q.bindValue(QStringLiteral(":order"), order);
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

bool TaskRepository::toggleChecklistItem(qint64 id, int index, bool checked)
{
    models::Task t = byId(id);
    if (t.id < 0 || index < 0 || index >= t.checklist.size()) return false;
    const QString item = t.checklist.at(index);
    if (checked && !t.checkedItems.contains(item)) t.checkedItems.append(item);
    if (!checked) t.checkedItems.removeAll(item);
    return update(t);
}

int TaskRepository::rollRecurring(const QDate& today)
{
    int created = 0;
    const QVector<models::Task> tasks = active();
    for (const models::Task& t : tasks) {
        if (t.recurring.isEmpty() || !t.dueAt.isValid()) continue;
        const QDate due = t.dueAt.date();
        if (due >= today) continue;

        QDate next;
        if (t.recurring == QLatin1String("daily")) {
            next = due.addDays(1);
        } else if (t.recurring == QLatin1String("weekly")) {
            next = due.addDays(7);
        } else if (t.recurring == QLatin1String("monthly")) {
            next = due.addMonths(1);
        } else {
            next = due.addDays(1);
        }
        while (next < today) {
            if (t.recurring == QLatin1String("daily")) next = next.addDays(1);
            else if (t.recurring == QLatin1String("weekly")) next = next.addDays(7);
            else next = next.addMonths(1);
        }
        if (next > today) next = today; // catch-up: don't queue stale dates

        models::Task copy = t;
        copy.id = -1;
        copy.dueAt = QDateTime(next, QTime(17, 0));
        copy.checkedItems.clear();
        copy.status = models::TaskStatus::Active;
        copy.completedAt = QDateTime();
        copy.createdAt = QDateTime::currentDateTime();
        if (insert(copy) > 0) ++created;
    }
    return created;
}

} // namespace arete::data
