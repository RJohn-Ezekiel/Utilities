#pragma once

#include <QObject>
#include <QTimer>

#include "core/TimerState.h"
#include "models/Settings.h"

namespace chronos {

class ReminderScheduler : public QObject {
    Q_OBJECT

public:
    explicit ReminderScheduler(QObject* parent = nullptr);

    void configure(const Settings& settings);
    void startAll();
    void stopAll();

signals:
    void reminderTriggered(HealthReminderType type);

private:
    void setupTimer(QTimer*& timer, int intervalSec, HealthReminderType type);

    QTimer* m_waterTimer = nullptr;
    QTimer* m_standTimer = nullptr;
    QTimer* m_stretchTimer = nullptr;
    QTimer* m_eyeTimer = nullptr;

    int m_waterInterval = 1800;
    int m_standInterval = 1800;
    int m_stretchInterval = 3600;
    int m_eyeInterval = 1200;
};

} // namespace chronos
