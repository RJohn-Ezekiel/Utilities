#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>

#include "TimerState.h"

namespace chronos {

class TimerEngine : public QObject {
    Q_OBJECT

public:
    explicit TimerEngine(QObject* parent = nullptr);
    ~TimerEngine() override;

    TimerState state() const;
    SessionType currentSessionType() const;
    int remainingSeconds() const;
    int elapsedSeconds() const;
    int totalSeconds() const;
    QDateTime sessionStartTime() const;

public slots:
    void start(SessionType type, int durationSeconds);
    void pause();
    void resume();
    void stop();
    void skipBreak();
    void restore(TimerState state, SessionType sessionType,
                 int remainingSeconds, int totalSeconds);
    void reset();

signals:
    void stateChanged(TimerState state, SessionType sessionType);
    void tick(int remainingSeconds, int elapsedSeconds, int totalSeconds);
    void sessionCompleted(SessionType type);

private slots:
    void onTick();

private:
    void setState(TimerState newState);
    void startTimer();
    void stopTimer();

    QTimer* m_timer = nullptr;
    TimerState m_state = TimerState::Idle;
    SessionType m_currentSessionType = SessionType::Focus;
    int m_remainingSeconds = 0;
    int m_elapsedSeconds = 0;
    int m_totalSeconds = 0;
    QDateTime m_sessionStartTime;
};

} // namespace chronos
