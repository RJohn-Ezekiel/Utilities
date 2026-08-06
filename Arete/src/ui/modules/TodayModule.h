#pragma once

#include "core/Module.h"

#include <QWidget>
#include <QDate>

class QListWidget;
class QPushButton;
class QLabel;
class QProgressBar;

namespace arete {
namespace app { class AppContext; }
namespace models { struct Task; }

namespace ui {

// The Today tab — a port of cron's "Daily Dashboard": a summary panel with
// Issues/Habits progress, a momentum/signals block, and the day's issues
// and habits side by side. Supports prev/next/today date navigation.
class TodayModule : public core::Module
{
    Q_OBJECT

public:
    explicit TodayModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("today"); }
    QString moduleTitle() const override { return QStringLiteral("Today"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;

public slots:
    void highlightTask(qint64 taskId);

private:
    void refresh();
    void refreshSummary();
    void refreshIssues();
    void refreshHabits();
    void appendTask(const models::Task& task);
    void goToDate(const QDate& date);
    static QString sectionTitle(const QString& label, int count);

    QDate m_selectedDate = QDate::currentDate();
    QLabel* m_dateLabel;
    QPushButton* m_prevButton;
    QPushButton* m_nextButton;
    QPushButton* m_todayButton;
    QLabel* m_issuesSummary;
    QProgressBar* m_issuesBar;
    QLabel* m_habitsSummary;
    QProgressBar* m_habitsBar;
    QLabel* m_signalsLabel;
    QLabel* m_streaksLabel;
    QPushButton* m_addButton;
    QListWidget* m_issueList;
    QListWidget* m_habitList;
    QDate m_lastRefresh;
};

} // namespace ui
} // namespace arete