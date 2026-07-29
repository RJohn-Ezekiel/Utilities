#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QCheckBox>

#include "models/Settings.h"
#include "storage/StorageManager.h"

namespace chronos {

class TimerService;
class ReminderScheduler;

class SettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SettingsWidget(StorageManager* storage, TimerService* timerService,
                            ReminderScheduler* reminderScheduler,
                            QWidget* parent = nullptr);

    void loadSettings();

signals:
    void settingsChanged();
    void resetAllRequested();

private:
    void setupUi();
    void saveSettings();
    void applyLaunchAtStartup(bool enable);

    StorageManager* m_storage = nullptr;
    TimerService* m_timerService = nullptr;
    ReminderScheduler* m_reminderScheduler = nullptr;
    Settings m_original;

    // Timer
    QSpinBox* m_focusMin = nullptr;
    QSpinBox* m_shortBreakMin = nullptr;
    QSpinBox* m_longBreakMin = nullptr;
    QSpinBox* m_sessionsBeforeLB = nullptr;

    // Reminders
    QSpinBox* m_waterMin = nullptr;
    QSpinBox* m_standMin = nullptr;
    QSpinBox* m_stretchMin = nullptr;
    QSpinBox* m_eyeMin = nullptr;

    // Appearance
    QSpinBox* m_fontSize = nullptr;

    // Behaviour
    QCheckBox* m_soundEnabled = nullptr;
    QCheckBox* m_alwaysOnTop = nullptr;
    QCheckBox* m_launchAtStartup = nullptr;
    QCheckBox* m_rememberSize = nullptr;
    QCheckBox* m_rememberSession = nullptr;
};

} // namespace chronos
