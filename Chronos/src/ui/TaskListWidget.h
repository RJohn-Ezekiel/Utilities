#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QList>

namespace chronos {

class TaskService;
class TaskWidget;

class TaskListWidget : public QWidget {
    Q_OBJECT

public:
    explicit TaskListWidget(TaskService* taskService, QWidget* parent = nullptr);

signals:
    void startFocusRequested(const QString& taskId);

private slots:
    void onTasksChanged();
    void onAddTask();

    void onStartTask(const QString& taskId);
    void onCompleteToggled(const QString& taskId, bool completed);
    void onEditTask(const QString& taskId);
    void onDeleteTask(const QString& taskId);
    void onMoveUp(const QString& taskId);
    void onMoveDown(const QString& taskId);

private:
    void rebuildTaskWidgets();
    static bool editTaskDialog(QWidget* parent, QString& title,
                               QString& description, int& estimatedSessions);

    TaskService* m_taskService = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_scrollContent = nullptr;
    QVBoxLayout* m_taskLayout = nullptr;
    QPushButton* m_addBtn = nullptr;
    QList<TaskWidget*> m_taskWidgets;
};

} // namespace chronos
