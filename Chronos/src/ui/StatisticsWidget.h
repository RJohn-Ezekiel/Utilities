#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>

#include "services/StatisticsService.h"

namespace chronos {

class StatisticsWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatisticsWidget(StatisticsService* statsService,
                              QWidget* parent = nullptr);

    void refresh();

private:
    void showWeek();
    void showMonth();
    void updateDisplay(const QString& period, const DailyStats& stats);

    StatisticsService* m_statsService;
    QLabel* m_periodLabel = nullptr;
    QLabel* m_focusLabel = nullptr;
    QLabel* m_breakLabel = nullptr;
    QLabel* m_sessionsLabel = nullptr;
    QLabel* m_tasksLabel = nullptr;
    QLabel* m_longestLabel = nullptr;
    QLabel* m_avgLabel = nullptr;
    QLabel* m_chartPlaceholder = nullptr;
    QPushButton* m_weekBtn = nullptr;
    QPushButton* m_monthBtn = nullptr;
    bool m_showingWeek = true;
};

} // namespace chronos
