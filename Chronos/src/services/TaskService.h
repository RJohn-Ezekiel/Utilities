#pragma once

#include <QObject>
#include <QList>

#include "models/Task.h"
#include "storage/StorageManager.h"

namespace chronos {

class TaskService : public QObject {
    Q_OBJECT

public:
    explicit TaskService(StorageManager* storage, QObject* parent = nullptr);

    const QList<Task>& tasks() const;
    Task task(const QString& id) const;

    Task addTask(const QString& title, const QString& description,
                 int estimatedSessions = 1);
    void editTask(const QString& id, const QString& title,
                  const QString& description, int estimatedSessions);
    void deleteTask(const QString& id);
    void completeTask(const QString& id);
    void uncompleteTask(const QString& id);
    void moveTaskUp(const QString& id);
    void moveTaskDown(const QString& id);
    void incrementCompletedSessions(const QString& taskId);

    QList<Task> incompleteTasks() const;
    QList<Task> completedToday() const;

signals:
    void tasksChanged();

private:
    void save();
    int indexOf(const QString& id) const;

    StorageManager* m_storage = nullptr;
    QList<Task> m_tasks;
};

} // namespace chronos
