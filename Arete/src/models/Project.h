#pragma once

#include <QString>
#include <QDateTime>

namespace arete::models {

struct Project
{
    qint64 id = -1;
    QString name;
    QString description;
    QDateTime deadline;
    bool archived = false;
    QDateTime createdAt;

    [[nodiscard]] QString milestoneSummary() const { return name; }
};

} // namespace arete::models
