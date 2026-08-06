#pragma once

#include <QString>
#include <QDateTime>
#include <QDate>
#include <QVariant>

namespace arete::models {

enum class TaskStatus { Active, Completed, Archived };
enum class Priority { Low, Medium, High };

// A unit of work. Can belong to a project, recur, and carry a checklist.
struct Task
{
    qint64 id = -1;
    QString title;
    QString notes;
    qint64 projectId = 0;
    Priority priority = Priority::Medium;
    TaskStatus status = TaskStatus::Active;
    QDateTime dueAt;
    QString recurring;          // e.g. "daily", "weekly", "monthly"
    QStringList checklist;      // raw checklist items, one per line
    QStringList checkedItems;   // items already checked
    qint64 sortOrder = 0;
    QDateTime createdAt;
    QDateTime completedAt;

    [[nodiscard]] bool isOverdue() const
    {
        return status == TaskStatus::Active && dueAt.isValid() && dueAt.date() < QDate::currentDate();
    }

    [[nodiscard]] bool isDueToday() const
    {
        return status == TaskStatus::Active && dueAt.isValid() && dueAt.date() == QDate::currentDate();
    }

    [[nodiscard]] double completion() const
    {
        if (checklist.isEmpty()) return (status == TaskStatus::Completed) ? 1.0 : 0.0;
        return static_cast<double>(checkedItems.size()) / static_cast<double>(checklist.size());
    }
};

} // namespace arete::models
