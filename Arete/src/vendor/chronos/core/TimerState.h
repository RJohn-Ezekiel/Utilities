#pragma once

#include <QObject>

namespace chronos {

Q_NAMESPACE

enum class TimerState {
    Idle,
    Focusing,
    ShortBreak,
    LongBreak,
    Paused
};

enum class SessionType {
    Focus,
    ShortBreak,
    LongBreak
};

enum class HealthReminderType {
    Water,
    Stand,
    Stretch,
    Eye
};

} // namespace chronos
