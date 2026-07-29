#pragma once

#include <QObject>
#include <QString>
#include <QList>

#include "models/Settings.h"
#include "models/Task.h"
#include "models/Session.h"
#include "models/Statistics.h"

namespace chronos {

struct SessionState {
    int remainingSeconds = 0;
    int completedSessions = 0;
    int currentState = 0;
    int currentSessionType = 0;
};

class StorageManager : public QObject {
    Q_OBJECT

public:
    explicit StorageManager(QObject* parent = nullptr);

    void saveSettings(const Settings& settings);
    Settings loadSettings();

    void saveTasks(const QList<Task>& tasks);
    QList<Task> loadTasks();

    void saveSessions(const QList<Session>& sessions);
    QList<Session> loadSessions();

    void saveStatistics(const Statistics& stats);
    Statistics loadStatistics();

    void saveSessionState(const SessionState& state);
    SessionState loadSessionState();
    void clearSessionState();
    void clearAll();

    QString dataDirectory() const;

private:
    QString m_dataDir;

    QString settingsPath() const;
    QString tasksPath() const;
    QString sessionsPath() const;
    QString statisticsPath() const;
    QString sessionStatePath() const;

    void ensureDataDirExists() const;
};

} // namespace chronos
