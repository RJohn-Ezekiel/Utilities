#include "TimerEngine.h"

namespace chronos {

TimerEngine::TimerEngine(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &TimerEngine::onTick);
}

TimerEngine::~TimerEngine() = default;

TimerState TimerEngine::state() const { return m_state; }
SessionType TimerEngine::currentSessionType() const { return m_currentSessionType; }
int TimerEngine::remainingSeconds() const { return m_remainingSeconds; }
int TimerEngine::elapsedSeconds() const { return m_elapsedSeconds; }
int TimerEngine::totalSeconds() const { return m_totalSeconds; }
QDateTime TimerEngine::sessionStartTime() const { return m_sessionStartTime; }

void TimerEngine::start(SessionType type, int durationSeconds)
{
    stopTimer();
    m_currentSessionType = type;
    m_remainingSeconds = durationSeconds;
    m_elapsedSeconds = 0;
    m_totalSeconds = durationSeconds;
    m_sessionStartTime = QDateTime::currentDateTime();

    switch (type) {
    case SessionType::Focus:
        setState(TimerState::Focusing);
        break;
    case SessionType::ShortBreak:
        setState(TimerState::ShortBreak);
        break;
    case SessionType::LongBreak:
        setState(TimerState::LongBreak);
        break;
    }
    startTimer();
}

void TimerEngine::pause()
{
    if (m_state == TimerState::Focusing
        || m_state == TimerState::ShortBreak
        || m_state == TimerState::LongBreak) {
        stopTimer();
        setState(TimerState::Paused);
    }
}

void TimerEngine::resume()
{
    if (m_state == TimerState::Paused) {
        switch (m_currentSessionType) {
        case SessionType::Focus:
            setState(TimerState::Focusing);
            break;
        case SessionType::ShortBreak:
            setState(TimerState::ShortBreak);
            break;
        case SessionType::LongBreak:
            setState(TimerState::LongBreak);
            break;
        }
        startTimer();
    }
}

void TimerEngine::stop()
{
    stopTimer();
    reset();
}

void TimerEngine::skipBreak()
{
    if (m_state == TimerState::ShortBreak
        || m_state == TimerState::LongBreak) {
        stopTimer();
        emit sessionCompleted(m_currentSessionType);
        reset();
    }
}

void TimerEngine::restore(TimerState state, SessionType sessionType,
                         int remainingSeconds, int totalSeconds)
{
    stopTimer();
    if (state == TimerState::Idle) {
        reset();
        return;
    }

    m_currentSessionType = sessionType;
    m_remainingSeconds = remainingSeconds;
    m_elapsedSeconds = totalSeconds - remainingSeconds;
    m_totalSeconds = totalSeconds;
    m_sessionStartTime = QDateTime::currentDateTime().addSecs(-m_elapsedSeconds);
    setState(state);

    if (state != TimerState::Paused) {
        startTimer();
    }
}

void TimerEngine::reset()
{
    stopTimer();
    m_remainingSeconds = 0;
    m_elapsedSeconds = 0;
    m_totalSeconds = 0;
    setState(TimerState::Idle);
}

void TimerEngine::onTick()
{
    if (m_remainingSeconds > 0) {
        m_remainingSeconds--;
        m_elapsedSeconds++;
        emit tick(m_remainingSeconds, m_elapsedSeconds, m_totalSeconds);

        if (m_remainingSeconds == 0) {
            stopTimer();
            emit sessionCompleted(m_currentSessionType);
        }
    }
}

void TimerEngine::setState(TimerState newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(m_state, m_currentSessionType);
    }
}

void TimerEngine::startTimer()
{
    if (!m_timer->isActive()) {
        m_timer->start();
    }
}

void TimerEngine::stopTimer()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
}

} // namespace chronos
