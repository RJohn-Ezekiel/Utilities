#include "StorageManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>

namespace chronos {

StorageManager::StorageManager(QObject* parent)
    : QObject(parent)
{
    m_dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    ensureDataDirExists();
}

void StorageManager::ensureDataDirExists() const
{
    QDir dir(m_dataDir);
    if (!dir.exists()) {
        dir.mkpath(m_dataDir);
    }
}

QString StorageManager::dataDirectory() const
{
    return m_dataDir;
}

// --- Paths ---

QString StorageManager::settingsPath() const
{
    return m_dataDir + QStringLiteral("/settings.json");
}

QString StorageManager::tasksPath() const
{
    return m_dataDir + QStringLiteral("/tasks.json");
}

QString StorageManager::sessionsPath() const
{
    return m_dataDir + QStringLiteral("/sessions.json");
}

QString StorageManager::statisticsPath() const
{
    return m_dataDir + QStringLiteral("/statistics.json");
}

QString StorageManager::sessionStatePath() const
{
    return m_dataDir + QStringLiteral("/session_state.json");
}

// --- Settings ---

void StorageManager::saveSettings(const Settings& settings)
{
    QJsonObject obj;
    obj[QStringLiteral("focusDuration")] = settings.focusDuration;
    obj[QStringLiteral("shortBreakDuration")] = settings.shortBreakDuration;
    obj[QStringLiteral("longBreakDuration")] = settings.longBreakDuration;
    obj[QStringLiteral("sessionsBeforeLongBreak")] = settings.sessionsBeforeLongBreak;
    obj[QStringLiteral("waterReminderInterval")] = settings.waterReminderInterval;
    obj[QStringLiteral("standReminderInterval")] = settings.standReminderInterval;
    obj[QStringLiteral("stretchReminderInterval")] = settings.stretchReminderInterval;
    obj[QStringLiteral("eyeReminderInterval")] = settings.eyeReminderInterval;
    obj[QStringLiteral("fontSize")] = settings.fontSize;
    obj[QStringLiteral("notificationSound")] = settings.notificationSound;
    obj[QStringLiteral("alwaysOnTop")] = settings.alwaysOnTop;
    obj[QStringLiteral("launchAtStartup")] = settings.launchAtStartup;
    obj[QStringLiteral("rememberWindowSize")] = settings.rememberWindowSize;
    obj[QStringLiteral("rememberSession")] = settings.rememberSession;
    obj[QStringLiteral("windowWidth")] = settings.windowWidth;
    obj[QStringLiteral("windowHeight")] = settings.windowHeight;

    QFile file(settingsPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    }
}

Settings StorageManager::loadSettings()
{
    Settings settings;
    QFile file(settingsPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return settings;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return settings;
    }

    QJsonObject obj = doc.object();
    settings.focusDuration = obj[QStringLiteral("focusDuration")].toInt(settings.focusDuration);
    settings.shortBreakDuration = obj[QStringLiteral("shortBreakDuration")].toInt(settings.shortBreakDuration);
    settings.longBreakDuration = obj[QStringLiteral("longBreakDuration")].toInt(settings.longBreakDuration);
    settings.sessionsBeforeLongBreak = obj[QStringLiteral("sessionsBeforeLongBreak")].toInt(settings.sessionsBeforeLongBreak);
    settings.waterReminderInterval = obj[QStringLiteral("waterReminderInterval")].toInt(settings.waterReminderInterval);
    settings.standReminderInterval = obj[QStringLiteral("standReminderInterval")].toInt(settings.standReminderInterval);
    settings.stretchReminderInterval = obj[QStringLiteral("stretchReminderInterval")].toInt(settings.stretchReminderInterval);
    settings.eyeReminderInterval = obj[QStringLiteral("eyeReminderInterval")].toInt(settings.eyeReminderInterval);
    settings.fontSize = obj[QStringLiteral("fontSize")].toInt(settings.fontSize);
    settings.notificationSound = obj[QStringLiteral("notificationSound")].toBool(settings.notificationSound);
    settings.alwaysOnTop = obj[QStringLiteral("alwaysOnTop")].toBool(settings.alwaysOnTop);
    settings.launchAtStartup = obj[QStringLiteral("launchAtStartup")].toBool(settings.launchAtStartup);
    settings.rememberWindowSize = obj[QStringLiteral("rememberWindowSize")].toBool(settings.rememberWindowSize);
    settings.rememberSession = obj[QStringLiteral("rememberSession")].toBool(settings.rememberSession);
    settings.windowWidth = obj[QStringLiteral("windowWidth")].toInt(settings.windowWidth);
    settings.windowHeight = obj[QStringLiteral("windowHeight")].toInt(settings.windowHeight);

    return settings;
}

// --- Tasks ---

void StorageManager::saveTasks(const QList<Task>& tasks)
{
    QJsonArray arr;
    for (const auto& task : tasks) {
        QJsonObject obj;
        obj[QStringLiteral("id")] = task.id;
        obj[QStringLiteral("title")] = task.title;
        obj[QStringLiteral("description")] = task.description;
        obj[QStringLiteral("estimatedSessions")] = task.estimatedSessions;
        obj[QStringLiteral("completedSessions")] = task.completedSessions;
        obj[QStringLiteral("completed")] = task.completed;
        obj[QStringLiteral("createdAt")] = task.createdAt.toString(Qt::ISODate);
        obj[QStringLiteral("completedAt")] = task.completedAt.isValid()
            ? task.completedAt.toString(Qt::ISODate)
            : QString();
        obj[QStringLiteral("position")] = task.position;
        arr.append(obj);
    }

    QFile file(tasksPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    }
}

QList<Task> StorageManager::loadTasks()
{
    QList<Task> tasks;
    QFile file(tasksPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return tasks;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return tasks;
    }

    const QJsonArray arr = doc.array();
    tasks.reserve(arr.size());
    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        Task task;
        task.id = obj[QStringLiteral("id")].toString();
        task.title = obj[QStringLiteral("title")].toString();
        task.description = obj[QStringLiteral("description")].toString();
        task.estimatedSessions = obj[QStringLiteral("estimatedSessions")].toInt(1);
        task.completedSessions = obj[QStringLiteral("completedSessions")].toInt(0);
        task.completed = obj[QStringLiteral("completed")].toBool(false);
        QString createdAtStr = obj[QStringLiteral("createdAt")].toString();
        if (!createdAtStr.isEmpty()) {
            task.createdAt = QDateTime::fromString(createdAtStr, Qt::ISODate);
        }
        QString completedAtStr = obj[QStringLiteral("completedAt")].toString();
        if (!completedAtStr.isEmpty()) {
            task.completedAt = QDateTime::fromString(completedAtStr, Qt::ISODate);
        }
        task.position = obj[QStringLiteral("position")].toInt(0);
        tasks.append(task);
    }

    return tasks;
}

// --- Sessions ---

void StorageManager::saveSessions(const QList<Session>& sessions)
{
    QJsonArray arr;
    for (const auto& session : sessions) {
        QJsonObject obj;
        obj[QStringLiteral("id")] = session.id;
        obj[QStringLiteral("taskId")] = session.taskId;
        obj[QStringLiteral("type")] = static_cast<int>(session.type);
        obj[QStringLiteral("startTime")] = session.startTime.toString(Qt::ISODate);
        obj[QStringLiteral("endTime")] = session.endTime.toString(Qt::ISODate);
        obj[QStringLiteral("durationSeconds")] = session.durationSeconds;
        obj[QStringLiteral("completed")] = session.completed;
        obj[QStringLiteral("note")] = session.note;
        arr.append(obj);
    }

    QFile file(sessionsPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    }
}

QList<Session> StorageManager::loadSessions()
{
    QList<Session> sessions;
    QFile file(sessionsPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return sessions;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
        return sessions;
    }

    const QJsonArray arr = doc.array();
    sessions.reserve(arr.size());
    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        Session session;
        session.id = obj[QStringLiteral("id")].toString();
        session.taskId = obj[QStringLiteral("taskId")].toString();
        session.type = static_cast<SessionType>(obj[QStringLiteral("type")].toInt(0));
        QString startStr = obj[QStringLiteral("startTime")].toString();
        if (!startStr.isEmpty()) {
            session.startTime = QDateTime::fromString(startStr, Qt::ISODate);
        }
        QString endStr = obj[QStringLiteral("endTime")].toString();
        if (!endStr.isEmpty()) {
            session.endTime = QDateTime::fromString(endStr, Qt::ISODate);
        }
        session.durationSeconds = obj[QStringLiteral("durationSeconds")].toInt(0);
        session.completed = obj[QStringLiteral("completed")].toBool(false);
        session.note = obj[QStringLiteral("note")].toString();
        sessions.append(session);
    }

    return sessions;
}

// --- Statistics ---

void StorageManager::saveStatistics(const Statistics& stats)
{
    QJsonObject todayObj;
    todayObj[QStringLiteral("focusSeconds")] = stats.today.focusSeconds;
    todayObj[QStringLiteral("breakSeconds")] = stats.today.breakSeconds;
    todayObj[QStringLiteral("sessionsCompleted")] = stats.today.sessionsCompleted;
    todayObj[QStringLiteral("tasksCompleted")] = stats.today.tasksCompleted;
    todayObj[QStringLiteral("waterAcks")] = stats.today.waterAcks;
    todayObj[QStringLiteral("waterGlasses")] = stats.today.waterGlasses;
    todayObj[QStringLiteral("standAcks")] = stats.today.standAcks;
    todayObj[QStringLiteral("stretchAcks")] = stats.today.stretchAcks;
    todayObj[QStringLiteral("eyeAcks")] = stats.today.eyeAcks;

    QJsonObject historyObj;
    for (auto it = stats.dailyHistory.constBegin(); it != stats.dailyHistory.constEnd(); ++it) {
        QJsonObject dObj;
        dObj[QStringLiteral("focusSeconds")] = it.value().focusSeconds;
        dObj[QStringLiteral("breakSeconds")] = it.value().breakSeconds;
        dObj[QStringLiteral("sessionsCompleted")] = it.value().sessionsCompleted;
        dObj[QStringLiteral("tasksCompleted")] = it.value().tasksCompleted;
        dObj[QStringLiteral("waterAcks")] = it.value().waterAcks;
        dObj[QStringLiteral("waterGlasses")] = it.value().waterGlasses;
        dObj[QStringLiteral("standAcks")] = it.value().standAcks;
        dObj[QStringLiteral("stretchAcks")] = it.value().stretchAcks;
        dObj[QStringLiteral("eyeAcks")] = it.value().eyeAcks;
        historyObj[it.key().toString(Qt::ISODate)] = dObj;
    }

    QJsonObject obj;
    obj[QStringLiteral("today")] = todayObj;
    obj[QStringLiteral("longestSessionSeconds")] = stats.longestSessionSeconds;
    obj[QStringLiteral("averageDailyFocusSeconds")] = stats.averageDailyFocusSeconds;
    obj[QStringLiteral("currentStreakDays")] = stats.currentStreakDays;
    obj[QStringLiteral("currentStreakStart")] = stats.currentStreakStart.isValid()
        ? stats.currentStreakStart.toString(Qt::ISODate)
        : QString();
    obj[QStringLiteral("dailyHistory")] = historyObj;

    QFile file(statisticsPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    }
}

Statistics StorageManager::loadStatistics()
{
    Statistics stats;
    QFile file(statisticsPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return stats;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return stats;
    }

    QJsonObject obj = doc.object();

    QJsonObject todayObj = obj[QStringLiteral("today")].toObject();
    stats.today.focusSeconds = todayObj[QStringLiteral("focusSeconds")].toInteger(0);
    stats.today.breakSeconds = todayObj[QStringLiteral("breakSeconds")].toInteger(0);
    stats.today.sessionsCompleted = todayObj[QStringLiteral("sessionsCompleted")].toInt(0);
    stats.today.tasksCompleted = todayObj[QStringLiteral("tasksCompleted")].toInt(0);
    stats.today.waterAcks = todayObj[QStringLiteral("waterAcks")].toInt(0);
    stats.today.waterGlasses = todayObj[QStringLiteral("waterGlasses")].toInt(0);
    stats.today.standAcks = todayObj[QStringLiteral("standAcks")].toInt(0);
    stats.today.stretchAcks = todayObj[QStringLiteral("stretchAcks")].toInt(0);
    stats.today.eyeAcks = todayObj[QStringLiteral("eyeAcks")].toInt(0);

    stats.longestSessionSeconds = obj[QStringLiteral("longestSessionSeconds")].toInteger(0);
    stats.averageDailyFocusSeconds = obj[QStringLiteral("averageDailyFocusSeconds")].toDouble(0.0);
    stats.currentStreakDays = obj[QStringLiteral("currentStreakDays")].toInt(0);

    QString streakStartStr = obj[QStringLiteral("currentStreakStart")].toString();
    if (!streakStartStr.isEmpty()) {
        stats.currentStreakStart = QDate::fromString(streakStartStr, Qt::ISODate);
    }

    QJsonObject historyObj = obj[QStringLiteral("dailyHistory")].toObject();
    for (auto it = historyObj.constBegin(); it != historyObj.constEnd(); ++it) {
        QDate date = QDate::fromString(it.key(), Qt::ISODate);
        if (!date.isValid()) continue;

        QJsonObject dObj = it.value().toObject();
        DailyStats ds;
        ds.focusSeconds = dObj[QStringLiteral("focusSeconds")].toInteger(0);
        ds.breakSeconds = dObj[QStringLiteral("breakSeconds")].toInteger(0);
        ds.sessionsCompleted = dObj[QStringLiteral("sessionsCompleted")].toInt(0);
        ds.tasksCompleted = dObj[QStringLiteral("tasksCompleted")].toInt(0);
        ds.waterAcks = dObj[QStringLiteral("waterAcks")].toInt(0);
        ds.waterGlasses = dObj[QStringLiteral("waterGlasses")].toInt(0);
        ds.standAcks = dObj[QStringLiteral("standAcks")].toInt(0);
        ds.stretchAcks = dObj[QStringLiteral("stretchAcks")].toInt(0);
        ds.eyeAcks = dObj[QStringLiteral("eyeAcks")].toInt(0);
        stats.dailyHistory.insert(date, ds);
    }

    return stats;
}

// --- Session State ---

void StorageManager::saveSessionState(const SessionState& state)
{
    QJsonObject obj;
    obj[QStringLiteral("remainingSeconds")] = state.remainingSeconds;
    obj[QStringLiteral("completedSessions")] = state.completedSessions;
    obj[QStringLiteral("currentState")] = state.currentState;
    obj[QStringLiteral("currentSessionType")] = state.currentSessionType;

    QFile file(sessionStatePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    }
}

SessionState StorageManager::loadSessionState()
{
    SessionState state;
    QFile file(sessionStatePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return state;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return state;
    }

    QJsonObject obj = doc.object();
    state.remainingSeconds = obj[QStringLiteral("remainingSeconds")].toInt(0);
    state.completedSessions = obj[QStringLiteral("completedSessions")].toInt(0);
    state.currentState = obj[QStringLiteral("currentState")].toInt(0);
    state.currentSessionType = obj[QStringLiteral("currentSessionType")].toInt(0);

    return state;
}

void StorageManager::clearSessionState()
{
    QFile file(sessionStatePath());
    if (file.exists()) {
        file.remove();
    }
}

void StorageManager::clearAll()
{
    saveTasks({});
    saveSessions({});
    saveStatistics(Statistics{});
    saveSettings(Settings{});
    clearSessionState();
}

} // namespace chronos
