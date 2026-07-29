#pragma once

#include <QObject>
#include <QString>

#include "core/TimerEngine.h"
#include "storage/StorageManager.h"

namespace chronos {

class TimerService : public QObject {
    Q_OBJECT

public:
    explicit TimerService(StorageManager* storage, QObject* parent = nullptr);

    TimerState state() const;
    SessionType currentSessionType() const;
    int remainingSeconds() const;
    int elapsedSeconds() const;
    int totalSeconds() const;
    QString currentTaskId() const;
    int consecutiveFocusSessions() const;

    const Settings& currentSettings() const;
    StorageManager* storage() const;
    void reloadSettings();
    void attemptRestore();

public slots:
    void startFocus();
    void startFocusForTask(const QString& taskId);
    void pause();
    void resume();
    void stop();
    void skipBreak();
    void proceedToBreak(const QString& note);
    void skipAfterSession(const QString& note);

signals:
    void stateChanged(TimerState state, SessionType sessionType);
    void tick(int remainingSeconds, int elapsedSeconds, int totalSeconds);
    void focusSessionCompleted(const QString& sessionId,
                               const QString& taskId,
                               int durationSeconds);
    void breakCompleted();
    void sessionRestored(TimerState state, SessionType sessionType,
                         int remainingSeconds);

private slots:
    void onEngineStateChanged(TimerState state, SessionType sessionType);
    void onEngineTick(int remainingSeconds, int elapsedSeconds, int totalSeconds);
    void onSessionCompleted(SessionType type);

private:
    void startAppropriateBreak();
    QString recordSession(SessionType type, int durationSeconds, bool completed);
    void saveSessionState();

    TimerEngine* m_engine = nullptr;
    StorageManager* m_storage = nullptr;
    Settings m_settings;
    int m_consecutiveFocusSessions = 0;
    QString m_currentTaskId;
    QString m_pendingSessionId;
};

} // namespace chronos
