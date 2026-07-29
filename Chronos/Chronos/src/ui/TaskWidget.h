#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>

#include "models/Task.h"

namespace chronos {

class TaskWidget : public QWidget {
    Q_OBJECT

public:
    explicit TaskWidget(const Task& task, bool isFirst, bool isLast,
                        QWidget* parent = nullptr);

    void updateTask(const Task& task, bool isFirst, bool isLast);
    QString taskId() const;

signals:
    void startRequested(const QString& taskId);
    void completeToggled(const QString& taskId, bool completed);
    void editRequested(const QString& taskId);
    void deleteRequested(const QString& taskId);
    void moveUpRequested(const QString& taskId);
    void moveDownRequested(const QString& taskId);

private:
    void applyStyle();

    QString m_taskId;
    QPushButton* m_completeBtn = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_descriptionLabel = nullptr;
    QLabel* m_sessionsLabel = nullptr;
    QWidget* m_actionsWidget = nullptr;
    QPushButton* m_startBtn = nullptr;
    QPushButton* m_editBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QPushButton* m_moveUpBtn = nullptr;
    QPushButton* m_moveDownBtn = nullptr;
    bool m_completed = false;
};

} // namespace chronos
