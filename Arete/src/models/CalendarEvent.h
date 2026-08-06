#pragma once

#include <QString>
#include <QDateTime>

namespace arete::models {

enum class Recurrence { None, Daily, Weekly, Monthly, Yearly };

// A calendar event or time block.
struct CalendarEvent
{
    qint64 id = -1;
    QString title;
    QString notes;
    QDateTime start;
    QDateTime end;
    Recurrence recurrence = Recurrence::None;
    bool allDay = false;
    qint64 taskId = 0; // linked task deadline (0 = none)
    QDateTime createdAt;

    [[nodiscard]] bool overlaps(const QDateTime& from, const QDateTime& to) const
    {
        return start < to && end > from;
    }
};

} // namespace arete::models
