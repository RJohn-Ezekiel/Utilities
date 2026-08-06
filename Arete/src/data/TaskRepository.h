#pragma once

#include "models/Task.h"
#include <QObject>
#include <QVector>

namespace arete::data {

class DatabaseManager;

// Persistence for tasks, projects, and recurring rollover.
class TaskRepository : public QObject
{
    Q_OBJECT

public:
    explicit TaskRepository(DatabaseManager* db, QObject* parent = nullptr);

    [[nodiscard]] QVector<models::Task> all() const;
    [[nodiscard]] QVector<models::Task> active() const;
    [[nodiscard]] QVector<models::Task> forProject(qint64 projectId) const;
    [[nodiscard]] models::Task byId(qint64 id) const;

    qint64 insert(const models::Task& task);
    bool update(const models::Task& task);
    bool remove(qint64 id);
    bool setStatus(qint64 id, models::TaskStatus status, bool completed, const QDateTime& completedAt);
    bool setProject(qint64 id, qint64 projectId);
    bool setSortOrder(qint64 id, qint64 order);
    bool toggleChecklistItem(qint64 id, int index, bool checked);

    // Rolls recurring tasks over (duplicates due tasks into today).
    int rollRecurring(const QDate& today);

signals:
    void changed();

private:
    DatabaseManager* m_db;
};

} // namespace arete::data
