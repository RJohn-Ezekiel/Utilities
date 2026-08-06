#pragma once

#include "models/Checkin.h"
#include <QObject>
#include <QSet>
#include <QVector>

namespace arete::data {

class DatabaseManager;

// Persistence for daily wellbeing check-ins (mood / energy / note).
class CheckinRepository : public QObject
{
    Q_OBJECT

public:
    explicit CheckinRepository(DatabaseManager* db, QObject* parent = nullptr);

    // Insert or overwrite the check-in for the given date.
    bool upsert(const QDate& date, int mood, int energy, const QString& note);
    [[nodiscard]] models::Checkin entryFor(const QDate& date) const;
    [[nodiscard]] QVector<models::Checkin> range(const QDate& start, const QDate& end) const;
    [[nodiscard]] QSet<QDate> allDates() const;

signals:
    void changed();

private:
    DatabaseManager* m_db;
};

} // namespace arete::data