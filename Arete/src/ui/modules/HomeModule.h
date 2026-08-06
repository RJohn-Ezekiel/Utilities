#pragma once

#include "core/Module.h"
#include "services/VerseService.h"

#include <QWidget>
#include <QDate>

class QLabel;
class QProgressBar;
class QListWidget;
class QPushButton;
class QScrollArea;

namespace arete {
namespace app { class AppContext; }

namespace ui {

// The Home tab — a port of cron's "Summary": date header, a KPI band
// (focus / issues / habits / momentum), then Agenda and Signals columns.
class HomeModule : public core::Module
{
    Q_OBJECT

public:
    explicit HomeModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("home"); }
    QString moduleTitle() const override { return QStringLiteral("Home"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;

private:
    void refresh();
    void refreshKpi();
    void refreshCurrentTask();
    void refreshAgenda();
    void refreshSignals();
    void refreshGreeting();
    void startFocusOnCurrent();

    QLabel* m_greeting;
    QLabel* m_birthdayLabel;
    QLabel* m_dateLabel;
    QLabel* m_focusValue;
    QProgressBar* m_focusBar;
    QLabel* m_issuesValue;
    QProgressBar* m_issuesBar;
    QLabel* m_habitsValue;
    QProgressBar* m_habitsBar;
    QLabel* m_momentumValue;
    QProgressBar* m_momentumBar;
    QListWidget* m_agendaList;
    QListWidget* m_signalsList;
    QLabel* m_verseLabel;
    services::DailyVerse m_lastVerse;
    QLabel* m_currentTaskTitle;
    QProgressBar* m_progress;
    QPushButton* m_startFocus;
    QScrollArea* m_scroll;
};

} // namespace ui
} // namespace arete