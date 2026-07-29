#pragma once

#include <QDate>
#include <QMap>

namespace chronos {

struct DailyStats {
    qint64 focusSeconds = 0;
    qint64 breakSeconds = 0;
    int sessionsCompleted = 0;
    int tasksCompleted = 0;
    int waterAcks = 0;
    int waterGlasses = 0;
    int standAcks = 0;
    int stretchAcks = 0;
    int eyeAcks = 0;
};

struct Statistics {
    DailyStats today;
    qint64 longestSessionSeconds = 0;
    double averageDailyFocusSeconds = 0.0;
    int currentStreakDays = 0;
    QDate currentStreakStart;

    QMap<QDate, DailyStats> dailyHistory;
};

} // namespace chronos
