#pragma once

#include <QWidget>
#include <QLabel>
#include <QList>

#include "services/StatisticsService.h"

namespace chronos {

class StorageManager;
class WaterMeterWidget;

class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(StatisticsService* statsService,
                             StorageManager* storage,
                             QWidget* parent = nullptr);

    void refresh();

private:
    struct StatCard {
        QWidget* container = nullptr;
        QLabel* valueLabel = nullptr;
        QLabel* titleLabel = nullptr;
    };

    StatCard createCard(const QString& title, QWidget* parent);
    void setCardValue(StatCard& card, const QString& value);

    StatisticsService* m_statsService;
    WaterMeterWidget* m_waterMeter;
    StatCard m_focusCard;
    StatCard m_breakCard;
    StatCard m_sessionsCard;
    StatCard m_tasksCard;
    StatCard m_streakCard;
    StatCard m_waterCard;
    StatCard m_standCard;
    StatCard m_eyeCard;
};

} // namespace chronos
