#pragma once

#include <QDialog>
#include <QDate>
#include <QLabel>
#include <QPushButton>

class QDateEdit;

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Shown when a task is completed: "Task complete. What next?"
// Replaces a fixed end-of-day review.
class TaskCompleteDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Action { StartNext, ShortBreak, LongBreak, Reschedule, Journal, Dismiss };

    explicit TaskCompleteDialog(app::AppContext* context, QWidget* parent = nullptr);

    void setCompletedTask(const QString& title);
    [[nodiscard]] Action chosenAction() const { return m_action; }
    [[nodiscard]] QDate rescheduleDate() const { return m_rescheduleDate; }

private:
    void choose(Action action);

    app::AppContext* m_context;
    QLabel* m_title;
    QDateEdit* m_dateEdit = nullptr;
    Action m_action = Action::Dismiss;
    QDate m_rescheduleDate;
    QPushButton* m_startNext;
    QPushButton* m_shortBreak;
    QPushButton* m_longBreak;
    QPushButton* m_reschedule;
    QPushButton* m_journal;
};

} // namespace ui
} // namespace arete
