#pragma once

#include "models/Habit.h"
#include <QObject>
#include <QSet>
#include <QVector>

namespace arete::data {

class DatabaseManager;

// Persistence for habits and their daily completion logs. Streaks and
// weekly progress are computed from the log on each read.
class HabitRepository : public QObject
{
    Q_OBJECT

public:
    explicit HabitRepository(DatabaseManager* db, QObject* parent = nullptr);

    [[nodiscard]] QVector<models::Habit> all() const;
    [[nodiscard]] models::Habit byId(qint64 id) const;

    qint64 insert(const QString& name, int targetPerWeek);
    bool update(qint64 id, const QString& name, int targetPerWeek);
    bool remove(qint64 id);

    // Toggles the completion mark for a habit on a given day. Returns the
    // new state (true = marked done).
    bool toggle(qint64 habitId, const QDate& date);
    [[nodiscard]] bool isDone(qint64 habitId, const QDate& date) const;
    [[nodiscard]] QSet<QDate> doneDates(qint64 habitId) const;
    [[nodiscard]] int completedInRange(qint64 habitId, const QDate& start, const QDate& end) const;

signals:
    void changed();

private:
    struct Streaks
    {
        int current = 0;
        int longest = 0;
    };
    [[nodiscard]] Streaks computeStreaks(const QSet<QDate>& dates) const;

    DatabaseManager* m_db;
};

} // namespace arete::data