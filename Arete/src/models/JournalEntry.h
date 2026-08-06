#pragma once

#include <QString>
#include <QDate>

namespace arete::models {

struct JournalEntry
{
    qint64 id = -1;
    QDate date;
    QString reflection;
    QString gratitude;
    QString mistakes;
    QString lessons;
    QString scripture;
    QString tomorrow;
    QDateTime updatedAt;

    [[nodiscard]] bool isEmpty() const
    {
        return reflection.isEmpty() && gratitude.isEmpty() && mistakes.isEmpty()
            && lessons.isEmpty() && scripture.isEmpty() && tomorrow.isEmpty();
    }
};

} // namespace arete::models
