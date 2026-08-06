#pragma once

#include "models/Task.h"
#include "ui/common/TaskToggle.h"

#include <QWidget>
#include <QLabel>

namespace arete::ui::common {

// One task row: tick toggle, title, due/priority meta. Reused by Home, Today
// and the task completion flow.
class TaskRowWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TaskRowWidget(const models::Task& task, QWidget* parent = nullptr);

    [[nodiscard]] qint64 taskId() const { return m_task.id; }
    [[nodiscard]] models::Task task() const { return m_task; }
    void setTask(const models::Task& task);

    void setCheckable(bool checkable);

signals:
    void toggled(qint64 taskId, bool completed);
    void focusRequested(qint64 taskId, const QString& title);
    void editRequested(qint64 taskId);

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void refreshLabels();
    static QString priorityGlyph(models::Priority priority);

    models::Task m_task;
    TaskToggle* m_check;
    QLabel* m_title;
    QLabel* m_meta;
    bool m_checkable = true;
};

} // namespace arete::ui::common
