#include "data/CheckinRepository.h"
#include "data/DatabaseManager.h"
#include "arete/logging/Logger.h"

#include <QSqlQuery>
#include <QSqlError>

namespace arete::data {

using arete::logging::Logger;

namespace {
QString dateKey(const QDate& date)
{
    return date.toString(Qt::ISODate);
}
} // namespace

CheckinRepository::CheckinRepository(DatabaseManager* db, QObject* parent)
    : QObject(parent), m_db(db)
{
}

bool CheckinRepository::upsert(const QDate& date, int mood, int energy, const QString& note)
{
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral(
        "INSERT INTO checkins (date, mood, energy, note) VALUES (?, ?, ?, ?)"
        " ON CONFLICT(date) DO UPDATE SET mood = excluded.mood,"
        " energy = excluded.energy, note = excluded.note"));
    q.addBindValue(dateKey(date));
    q.addBindValue(mood);
    q.addBindValue(energy);
    q.addBindValue(note.trimmed());
    if (!q.exec()) {
        Logger::instance().error(QStringLiteral("Checkin upsert failed: %1").arg(q.lastError().text()),
                                 QStringLiteral("database"));
        return false;
    }
    emit changed();
    return true;
}

models::Checkin CheckinRepository::entryFor(const QDate& date) const
{
    models::Checkin result;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT id, date, mood, energy, note FROM checkins WHERE date = ?"));
    q.addBindValue(dateKey(date));
    if (!q.exec() || !q.next()) return result;
    result.id = q.value(0).toLongLong();
    result.date = QDate::fromString(q.value(1).toString(), Qt::ISODate);
    result.mood = q.value(2).toInt();
    result.energy = q.value(3).toInt();
    result.note = q.value(4).toString();
    return result;
}

QVector<models::Checkin> CheckinRepository::range(const QDate& start, const QDate& end) const
{
    QVector<models::Checkin> result;
    QSqlQuery q(m_db->connection());
    q.prepare(QStringLiteral("SELECT id, date, mood, energy, note FROM checkins"
                             " WHERE date >= ? AND date <= ? ORDER BY date"));
    q.addBindValue(dateKey(start));
    q.addBindValue(dateKey(end));
    if (!q.exec()) {
        Logger::instance().error(QStringLiteral("Checkin range failed: %1").arg(q.lastError().text()),
                                 QStringLiteral("database"));
        return result;
    }
    while (q.next()) {
        models::Checkin c;
        c.id = q.value(0).toLongLong();
        c.date = QDate::fromString(q.value(1).toString(), Qt::ISODate);
        c.mood = q.value(2).toInt();
        c.energy = q.value(3).toInt();
        c.note = q.value(4).toString();
        result.append(c);
    }
    return result;
}

QSet<QDate> CheckinRepository::allDates() const
{
    QSet<QDate> result;
    QSqlQuery q(m_db->connection());
    if (!q.exec(QStringLiteral("SELECT date FROM checkins"))) return result;
    while (q.next()) {
        const QDate d = QDate::fromString(q.value(0).toString(), Qt::ISODate);
        if (d.isValid()) result.insert(d);
    }
    return result;
}

} // namespace arete::data