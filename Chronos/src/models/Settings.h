#pragma once

#include <QString>
#include <QSize>

namespace chronos {

struct Settings {
    int focusDuration = 1500;
    int shortBreakDuration = 300;
    int longBreakDuration = 900;
    int sessionsBeforeLongBreak = 4;

    int waterReminderInterval = 1800;
    int standReminderInterval = 1800;
    int stretchReminderInterval = 1800;
    int eyeReminderInterval = 1800;

    int fontSize = 14;

    bool notificationSound = true;
    bool alwaysOnTop = false;
    bool launchAtStartup = false;
    bool rememberWindowSize = true;
    bool rememberSession = true;

    int windowWidth = 1024;
    int windowHeight = 768;
};

} // namespace chronos
