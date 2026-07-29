#pragma once

#include <QString>
#include <QDateTime>

#include "core/TimerState.h"

namespace chronos {

struct Session {
    QString id;
    QString taskId;
    SessionType type = SessionType::Focus;
    QDateTime startTime;
    QDateTime endTime;
    int durationSeconds = 0;
    bool completed = false;
    QString note;
};

} // namespace chronos
