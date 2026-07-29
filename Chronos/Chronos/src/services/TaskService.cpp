#include "TaskService.h"

#include <QUuid>
#include <QDateTime>

namespace chronos {

TaskService::TaskService(StorageManager* storage, QObject* parent)
    : QObject(parent)
    , m_storage(storage)
{
    m_tasks = m_storage->loadTasks();
}

const QList<Task>& TaskService::tasks() const
{
    return m_tasks;
}

Task TaskService::task(const QString& id) const
{
    for (const auto& t : m_tasks) {
        if (t.id == id) return t;
    }
    return Task{};
}

int TaskService::indexOf(const QString& id) const
{
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].id == id) return i;
    }
    return -1;
}

Task TaskService::addTask(const QString& title, const QString& description,
                          int estimatedSessions)
{
    Task task;
    task.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.title = title;
    task.description = description;
    task.estimatedSessions = qMax(1, estimatedSessions);
    task.completedSessions = 0;
    task.completed = false;
    task.createdAt = QDateTime::currentDateTime();
    task.position = m_tasks.size();

    m_tasks.append(task);
    save();
    emit tasksChanged();
    return task;
}

void TaskService::editTask(const QString& id, const QString& title,
                           const QString& description, int estimatedSessions)
{
    int idx = indexOf(id);
    if (idx < 0) return;

    m_tasks[idx].title = title;
    m_tasks[idx].description = description;
    m_tasks[idx].estimatedSessions = qMax(1, estimatedSessions);
    save();
    emit tasksChanged();
}

void TaskService::deleteTask(const QString& id)
{
    int idx = indexOf(id);
    if (idx < 0) return;

    m_tasks.removeAt(idx);
    save();
    emit tasksChanged();
}

void TaskService::completeTask(const QString& id)
{
    int idx = indexOf(id);
    if (idx < 0) return;

    m_tasks[idx].completed = true;
    m_tasks[idx].completedAt = QDateTime::currentDateTime();
    save();
    emit tasksChanged();
}

void TaskService::uncompleteTask(const QString& id)
{
    int idx = indexOf(id);
    if (idx < 0) return;

    m_tasks[idx].completed = false;
    m_tasks[idx].completedAt = QDateTime();
    save();
    emit tasksChanged();
}

void TaskService::moveTaskUp(const QString& id)
{
    int idx = indexOf(id);
    if (idx <= 0) return;

    m_tasks.swapItemsAt(idx, idx - 1);
    save();
    emit tasksChanged();
}

void TaskService::moveTaskDown(const QString& id)
{
    int idx = indexOf(id);
    if (idx < 0 || idx >= m_tasks.size() - 1) return;

    m_tasks.swapItemsAt(idx, idx + 1);
    save();
    emit tasksChanged();
}

void TaskService::incrementCompletedSessions(const QString& taskId)
{
    int idx = indexOf(taskId);
    if (idx < 0) return;

    m_tasks[idx].completedSessions++;
    save();
    emit tasksChanged();
}

QList<Task> TaskService::incompleteTasks() const
{
    QList<Task> result;
    for (const auto& t : m_tasks) {
        if (!t.completed) result.append(t);
    }
    return result;
}

QList<Task> TaskService::completedToday() const
{
    QList<Task> result;
    QDate today = QDate::currentDate();
    for (const auto& t : m_tasks) {
        if (t.completed && t.completedAt.date() == today) {
            result.append(t);
        }
    }
    return result;
}

void TaskService::save()
{
    m_storage->saveTasks(m_tasks);
}

} // namespace chronos
