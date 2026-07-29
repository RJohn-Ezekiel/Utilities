#include "TimerService.h"

#include <QUuid>

namespace chronos {

TimerService::TimerService(StorageManager* storage, QObject* parent)
    : QObject(parent)
    , m_engine(new TimerEngine(this))
    , m_storage(storage)
{
    m_settings = m_storage->loadSettings();

    connect(m_engine, &TimerEngine::stateChanged,
            this, &TimerService::onEngineStateChanged);
    connect(m_engine, &TimerEngine::tick,
            this, &TimerService::onEngineTick);
    connect(m_engine, &TimerEngine::sessionCompleted,
            this, &TimerService::onSessionCompleted);
}

// --- Accessors ---

TimerState TimerService::state() const { return m_engine->state(); }
SessionType TimerService::currentSessionType() const { return m_engine->currentSessionType(); }
int TimerService::remainingSeconds() const { return m_engine->remainingSeconds(); }
int TimerService::elapsedSeconds() const { return m_engine->elapsedSeconds(); }
int TimerService::totalSeconds() const { return m_engine->totalSeconds(); }
QString TimerService::currentTaskId() const { return m_currentTaskId; }
int TimerService::consecutiveFocusSessions() const { return m_consecutiveFocusSessions; }

const Settings& TimerService::currentSettings() const { return m_settings; }
StorageManager* TimerService::storage() const { return m_storage; }

// --- Settings ---

void TimerService::reloadSettings()
{
    m_settings = m_storage->loadSettings();
}

void TimerService::attemptRestore()
{
    if (!m_settings.rememberSession) {
        return;
    }

    SessionState state = m_storage->loadSessionState();
    if (state.currentState == static_cast<int>(TimerState::Idle)) {
        return;
    }

    m_consecutiveFocusSessions = state.completedSessions;

    auto timerState = static_cast<TimerState>(state.currentState);
    auto sessionType = static_cast<SessionType>(state.currentSessionType);
    m_engine->restore(timerState, sessionType,
                      state.remainingSeconds,
                      state.remainingSeconds); // total approximated from remaining

    emit sessionRestored(timerState, sessionType, state.remainingSeconds);
}

// --- Timer Control ---

void TimerService::startFocus()
{
    m_currentTaskId.clear();
    m_engine->start(SessionType::Focus, m_settings.focusDuration);
    saveSessionState();
}

void TimerService::startFocusForTask(const QString& taskId)
{
    m_currentTaskId = taskId;
    m_engine->start(SessionType::Focus, m_settings.focusDuration);
    saveSessionState();
}

void TimerService::pause()
{
    m_engine->pause();
    saveSessionState();
}

void TimerService::resume()
{
    m_engine->resume();
    saveSessionState();
}

void TimerService::stop()
{
    m_engine->stop();
    m_consecutiveFocusSessions = 0;
    m_currentTaskId.clear();
    m_storage->clearSessionState();
}

void TimerService::skipBreak()
{
    m_engine->skipBreak();
    m_storage->clearSessionState();
}

void TimerService::proceedToBreak(const QString& note)
{
    if (!m_pendingSessionId.isEmpty() && !note.isEmpty()) {
        auto sessions = m_storage->loadSessions();
        for (auto& s : sessions) {
            if (s.id == m_pendingSessionId) {
                s.note = note;
                break;
            }
        }
        m_storage->saveSessions(sessions);
    }
    m_pendingSessionId.clear();
    startAppropriateBreak();
}

void TimerService::skipAfterSession(const QString& note)
{
    if (!m_pendingSessionId.isEmpty() && !note.isEmpty()) {
        auto sessions = m_storage->loadSessions();
        for (auto& s : sessions) {
            if (s.id == m_pendingSessionId) {
                s.note = note;
                break;
            }
        }
        m_storage->saveSessions(sessions);
    }
    m_pendingSessionId.clear();
    m_engine->reset();
    m_consecutiveFocusSessions--;
    m_storage->clearSessionState();
    emit stateChanged(TimerState::Idle, SessionType::Focus);
}

// --- Internal Slots ---

void TimerService::onEngineStateChanged(TimerState state, SessionType sessionType)
{
    emit stateChanged(state, sessionType);
}

void TimerService::onEngineTick(int remainingSeconds, int elapsedSeconds, int totalSeconds)
{
    emit tick(remainingSeconds, elapsedSeconds, totalSeconds);
}

void TimerService::onSessionCompleted(SessionType type)
{
    if (type == SessionType::Focus) {
        m_consecutiveFocusSessions++;
        m_pendingSessionId = recordSession(type, m_engine->totalSeconds(), true);
        emit focusSessionCompleted(m_pendingSessionId, m_currentTaskId,
                                   m_engine->totalSeconds());
    } else {
        recordSession(type, m_engine->totalSeconds(), true);
        m_engine->reset();
        m_storage->clearSessionState();
        emit breakCompleted();
    }
}

// --- Private Helpers ---

void TimerService::startAppropriateBreak()
{
    if (m_consecutiveFocusSessions >= m_settings.sessionsBeforeLongBreak) {
        m_consecutiveFocusSessions = 0;
        m_engine->start(SessionType::LongBreak, m_settings.longBreakDuration);
    } else {
        m_engine->start(SessionType::ShortBreak, m_settings.shortBreakDuration);
    }
    saveSessionState();
}

QString TimerService::recordSession(SessionType type, int durationSeconds, bool completed)
{
    Session session;
    session.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    session.taskId = m_currentTaskId;
    session.type = type;
    session.startTime = m_engine->sessionStartTime();
    session.endTime = QDateTime::currentDateTime();
    session.durationSeconds = durationSeconds;
    session.completed = completed;

    auto sessions = m_storage->loadSessions();
    sessions.append(session);
    m_storage->saveSessions(sessions);

    return session.id;
}

void TimerService::saveSessionState()
{
    if (!m_settings.rememberSession) {
        return;
    }

    SessionState state;
    state.remainingSeconds = m_engine->remainingSeconds();
    state.completedSessions = m_consecutiveFocusSessions;
    state.currentState = static_cast<int>(m_engine->state());
    state.currentSessionType = static_cast<int>(m_engine->currentSessionType());
    m_storage->saveSessionState(state);
}

} // namespace chronos
