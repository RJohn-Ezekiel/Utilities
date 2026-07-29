#pragma once

#include <QObject>
#include <QDate>
#include <QMap>

#include "models/Statistics.h"
#include "storage/StorageManager.h"

namespace chronos {

class TaskService;

class StatisticsService : public QObject {
    Q_OBJECT

public:
    explicit StatisticsService(StorageManager* storage, TaskService* taskService,
                               QObject* parent = nullptr);

    Statistics compute();
    DailyStats computeForDateRange(const QDate& start, const QDate& end);
    int computeStreak();
    void recordHealthAck(HealthReminderType type);

signals:
    void statisticsUpdated();

private:
    void updateTodayStats();

    StorageManager* m_storage;
    TaskService* m_taskService;
    Statistics m_cache;
};

} // namespace chronos
