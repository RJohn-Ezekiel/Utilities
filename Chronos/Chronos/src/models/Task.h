#pragma once

#include <QString>
#include <QDateTime>

namespace chronos {

struct Task {
    QString id;
    QString title;
    QString description;
    int estimatedSessions = 1;
    int completedSessions = 0;
    bool completed = false;
    QDateTime createdAt;
    QDateTime completedAt;
    int position = 0;
};

} // namespace chronos
