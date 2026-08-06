#pragma once

#include <QObject>
#include <QVector>
#include <QMap>
#include <QDate>

namespace arete::data {

class DatabaseManager;

// Read-only aggregates over the focus_sessions table used by the
// Statistics module charts and the Home dashboard.
class StatsRepository : public QObject
{
    Q_OBJECT

public:
    explicit StatsRepository(DatabaseManager* db, QObject* parent = nullptr);

    // Focus minutes for each calendar day in [start, end] (index 0 = start).
    [[nodiscard]] QVector<int> focusMinutesPerDay(const QDate& start, const QDate& end) const;
    [[nodiscard]] int focusMinutesOn(const QDate& date) const;
    [[nodiscard]] int sessionsOn(const QDate& date) const;

    // Total focused minutes attributed to each project (unassigned = "").
    [[nodiscard]] QMap<QString, qint64> focusMinutesByProject(const QDate& start, const QDate& end) const;

    // Number of days (within [start, end]) with at least one focus session.
    [[nodiscard]] int activeDays(const QDate& start, const QDate& end) const;

private:
    DatabaseManager* m_db;
};

} // namespace arete::data