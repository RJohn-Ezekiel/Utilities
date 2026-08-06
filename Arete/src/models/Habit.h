#pragma once

#include <QString>
#include <QDate>
#include <QDateTime>

namespace arete::models {

// A habit the user wants to build. Completion is tracked per calendar day
// in the habit_logs table; streaks are derived, not stored.
struct Habit
{
    qint64 id = -1;
    QString name;
    int targetPerWeek = 5;   // goal: how many days per week
    QDateTime createdAt;

    // Convenience fields filled by HabitRepository queries.
    bool doneToday = false;
    int doneThisWeek = 0;
    int currentStreak = 0;
    int longestStreak = 0;
};

} // namespace arete::models