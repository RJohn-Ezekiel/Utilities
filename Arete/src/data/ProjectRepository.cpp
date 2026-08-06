#include "data/ProjectRepository.h"
#include "data/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace arete::data {

namespace {
models::Project readProject(const QSqlQuery& q)
{
    models::Project p;
    p.id = q.value(QStringLiteral("id")).toLongLong();
    p.name = q.value(QStringLiteral("name")).toString();
    p.description = q.value(QStringLiteral("description")).toString();
    p.deadline = QDateTime::fromString(q.value(QStringLiteral("deadline")).toString(), Qt::ISODate);
    p.archived = q.value(QStringLiteral("archived")).toBool();
    p.createdAt = QDateTime::fromString(q.value(QStringLiteral("created_at")).toString(), Qt::ISODate);
    return p;
}
} // namespace

ProjectRepository::ProjectRepository(DatabaseManager* db, QObject* parent)
    : QObject(parent), m_db(db)
{
}

QVector<models::Project> ProjectRepository::all(bool includeArchived) const
{
    QVector<models::Project> result;
    QSqlQuery q(m_db->connection());
    if (includeArchived) {
        q.prepare(QStringLiteral("SELECT * FROM projects ORDER BY name"));
    } else {
        q.prepare(QStringLiteral("SELECT * FROM projects WHERE archived = 0 ORDER BY name"));
    }
    if (!q.exec()) return result;
    while (q.next()) result.append(readProject(q));
    return result;
}

models::Project ProjectRepository::byId(qint64 id) const
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT * FROM projects WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next()) return {};
    return readProject(q);
}

qint64 ProjectRepository::insert(const models::Project& project)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("INSERT INTO projects (name, description, deadline, archived, created_at) "
                             "VALUES (:name, :description, :deadline, :archived, :created_at)"));
    q.bindValue(QStringLiteral(":name"), project.name);
    q.bindValue(QStringLiteral(":description"), project.description);
    q.bindValue(QStringLiteral(":deadline"),
                project.deadline.isValid() ? project.deadline.toString(Qt::ISODate) : QVariant());
    q.bindValue(QStringLiteral(":archived"), project.archived ? 1 : 0);
    q.bindValue(QStringLiteral(":created_at"),
                project.createdAt.isValid() ? project.createdAt.toString(Qt::ISODate)
                                            : QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) return -1;
    const qint64 id = q.lastInsertId().toLongLong();
    emit changed();
    return id;
}

bool ProjectRepository::update(const models::Project& project)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("UPDATE projects SET name=:name, description=:description, "
                             "deadline=:deadline, archived=:archived WHERE id=:id"));
    q.bindValue(QStringLiteral(":name"), project.name);
    q.bindValue(QStringLiteral(":description"), project.description);
    q.bindValue(QStringLiteral(":deadline"),
                project.deadline.isValid() ? project.deadline.toString(Qt::ISODate) : QVariant());
    q.bindValue(QStringLiteral(":archived"), project.archived ? 1 : 0);
    q.bindValue(QStringLiteral(":id"), project.id);
    const bool ok = q.exec();
    if (ok) emit changed();
    return ok;
}

bool ProjectRepository::remove(qint64 id)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("DELETE FROM projects WHERE id=:id"));
    q.bindValue(QStringLiteral(":id"), id);
    const bool ok = q.exec();
    if (ok) emit changed();
    return ok;
}

bool ProjectRepository::setArchived(qint64 id, bool archived)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("UPDATE projects SET archived=:archived WHERE id=:id"));
    q.bindValue(QStringLiteral(":archived"), archived ? 1 : 0);
    q.bindValue(QStringLiteral(":id"), id);
    const bool ok = q.exec();
    if (ok) emit changed();
    return ok;
}

} // namespace arete::data
