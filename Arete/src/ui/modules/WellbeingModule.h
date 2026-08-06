#pragma once

#include "core/Module.h"

#include <QWidget>
#include <QDate>

class QLabel;
class QPushButton;
class QSlider;
class QPlainTextEdit;
class QListWidget;
class LineChartWidget;
class ActivityHeatmapWidget;

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Wellbeing tab (Crona-inspired): daily mood/energy check-ins, a 14-day
// trend of both, and a focus activity heatmap for the last twelve weeks.
class WellbeingModule : public core::Module
{
    Q_OBJECT

public:
    explicit WellbeingModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("wellbeing"); }
    QString moduleTitle() const override { return QStringLiteral("Wellbeing"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;

private slots:
    void onSave();

private:
    void refreshHistory();
    void refreshCharts();

    QSlider* m_moodSlider;
    QSlider* m_energySlider;
    QLabel* m_moodValue;
    QLabel* m_energyValue;
    QPlainTextEdit* m_noteEdit;
    QPushButton* m_saveButton;
    QLabel* m_savedLabel;
    QListWidget* m_history;
    LineChartWidget* m_trend;
    ActivityHeatmapWidget* m_heatmap;
    QDate m_currentDate = QDate::currentDate();
};

} // namespace ui
} // namespace arete