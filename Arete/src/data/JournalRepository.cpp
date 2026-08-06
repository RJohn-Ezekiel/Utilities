#include "data/JournalRepository.h"
#include "data/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace arete::data {

namespace {
models::JournalEntry readEntry(const QSqlQuery& q)
{
    models::JournalEntry e;
    e.id = q.value(QStringLiteral("id")).toLongLong();
    e.date = QDate::fromString(q.value(QStringLiteral("date")).toString(), Qt::ISODate);
    e.reflection = q.value(QStringLiteral("reflection")).toString();
    e.gratitude = q.value(QStringLiteral("gratitude")).toString();
    e.mistakes = q.value(QStringLiteral("mistakes")).toString();
    e.lessons = q.value(QStringLiteral("lessons")).toString();
    e.scripture = q.value(QStringLiteral("scripture")).toString();
    e.tomorrow = q.value(QStringLiteral("tomorrow")).toString();
    e.updatedAt = QDateTime::fromString(q.value(QStringLiteral("updated_at")).toString(), Qt::ISODate);
    return e;
}
} // namespace

JournalRepository::JournalRepository(DatabaseManager* db, QObject* parent)
    : QObject(parent), m_db(db)
{
}

models::JournalEntry JournalRepository::forDate(const QDate& date) const
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT * FROM journal WHERE date = :date"));
    q.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));
    if (!q.exec() || !q.next()) return {};
    return readEntry(q);
}

QVector<models::JournalEntry> JournalRepository::recent(int limit) const
{
    QVector<models::JournalEntry> result;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT * FROM journal ORDER BY date DESC LIMIT :limit"));
    q.bindValue(QStringLiteral(":limit"), limit);
    if (!q.exec()) return result;
    while (q.next()) result.append(readEntry(q));
    return result;
}

QVector<models::JournalEntry> JournalRepository::search(const QString& term) const
{
    QVector<models::JournalEntry> result;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT * FROM journal WHERE reflection LIKE :term OR gratitude LIKE :term "
                             "OR mistakes LIKE :term OR lessons LIKE :term OR scripture LIKE :term "
                             "OR tomorrow LIKE :term ORDER BY date DESC LIMIT 50"));
    const QString pattern = QLatin1Char('%') + term + QLatin1Char('%');
    q.bindValue(QStringLiteral(":term"), pattern);
    if (!q.exec()) return result;
    while (q.next()) result.append(readEntry(q));
    return result;
}

qint64 JournalRepository::upsert(const models::JournalEntry& entry)
{
    models::JournalEntry existing = forDate(entry.date);
    QSqlQuery q(m_db->connection());
    if (existing.id < 0) {
        q.prepare(QStringLiteral("INSERT INTO journal (date, reflection, gratitude, mistakes, lessons, "
                                 "scripture, tomorrow, updated_at) VALUES (:date, :reflection, :gratitude, "
                                 ":mistakes, :lessons, :scripture, :tomorrow, :updated_at)"));
    } else {
        q.prepare(QStringLiteral("UPDATE journal SET reflection=:reflection, gratitude=:gratitude, "
                                 "mistakes=:mistakes, lessons=:lessons, scripture=:scripture, "
                                 "tomorrow=:tomorrow, updated_at=:updated_at WHERE id=:id"));
        q.bindValue(QStringLiteral(":id"), existing.id);
    }
    q.bindValue(QStringLiteral(":date"), entry.date.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":reflection"), entry.reflection);
    q.bindValue(QStringLiteral(":gratitude"), entry.gratitude);
    q.bindValue(QStringLiteral(":mistakes"), entry.mistakes);
    q.bindValue(QStringLiteral(":lessons"), entry.lessons);
    q.bindValue(QStringLiteral(":scripture"), entry.scripture);
    q.bindValue(QStringLiteral(":tomorrow"), entry.tomorrow);
    q.bindValue(QStringLiteral(":updated_at"), QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) return -1;
    const qint64 id = q.lastInsertId().isValid() ? q.lastInsertId().toLongLong() : existing.id;
    emit changed();
    return id;
}

bool JournalRepository::remove(const QDate& date)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("DELETE FROM journal WHERE date=:date"));
    q.bindValue(QStringLiteral(":date"), date.toString(Qt::ISODate));
    const bool ok = q.exec();
    if (ok) emit changed();
    return ok;
}

int JournalRepository::streak() const
{
    QSet<QDate> dates;
    QSqlQuery q(m_db->connection());
    if (!q.exec(QStringLiteral("SELECT date FROM journal"))) return 0;
    while (q.next()) dates.insert(QDate::fromString(q.value(0).toString(), Qt::ISODate));

    int streak = 0;
    QDate day = QDate::currentDate();
    if (!dates.contains(day)) day = day.addDays(-1); // yesterday counts if today not written yet
    while (dates.contains(day)) {
        ++streak;
        day = day.addDays(-1);
    }
    return streak;
}

} // namespace arete::data
