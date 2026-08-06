#pragma once

#include <QDate>
#include <QString>

namespace arete::models {

// A daily wellbeing check-in: mood and energy rated 1..5 plus a note.
// Stored one per day (date is UNIQUE); later ratings overwrite earlier ones.
struct Checkin
{
    qint64 id = -1;
    QDate date;
    int mood = 0;    // 1..5
    int energy = 0;  // 1..5
    QString note;
};

} // namespace arete::models