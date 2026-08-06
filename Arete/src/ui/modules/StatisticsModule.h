#pragma once

#include "core/Module.h"

#include <QWidget>

class QLabel;
class QListWidget;
class BarChartWidget;
class LineChartWidget;

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Statistics tab: today's totals, seven-day focus summary, weekly focus
// bars, habit completion, a mood/energy trend and a weekly momentum card —
// the Crona-inspired "summary" surface of the shell.
class StatisticsModule : public core::Module
{
    Q_OBJECT

public:
    explicit StatisticsModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("statistics"); }
    QString moduleTitle() const override { return QStringLiteral("Statistics"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;

private:
    void refresh();
    static QString fmtDuration(int minutes);
    static QString momentumLabel(int weeklyMinutes);

    QLabel* m_todayFocus;
    QLabel* m_todaySessions;
    QLabel* m_weekFocus;
    QLabel* m_streak;
    QLabel* m_momentumCard;
    QLabel* m_habitCard;
    QListWidget* m_weekList;
    BarChartWidget* m_focusBars;
    BarChartWidget* m_habitBars;
    LineChartWidget* m_moodTrend;
};

} // namespace ui
} // namespace arete