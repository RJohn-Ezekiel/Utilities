#include "data/DatabaseManager.h"
#include "arete/logging/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QVariant>

using arete::logging::Logger;

namespace arete::data {

class DatabaseManager::Private
{
public:
    explicit Private(const QString& path) : path(path) {}
    QString path;
    QSqlDatabase db;
    bool open = false;
};

DatabaseManager::DatabaseManager(const QString& databasePath, QObject* parent)
    : QObject(parent), d(std::make_unique<Private>(databasePath))
{
}

DatabaseManager::~DatabaseManager()
{
    if (d->open) {
        d->db.close();
    }
}

bool DatabaseManager::open()
{
    if (d->open) return true;

    const QDir dir(QFileInfo(d->path).absolutePath());
    if (!dir.exists() && !dir.mkpath(".")) {
        Logger::instance().error(QStringLiteral("Cannot create data directory: %1").arg(dir.absolutePath()),
                                 QStringLiteral("database"));
        return false;
    }

    d->db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("arete"));
    d->db.setDatabaseName(d->path);
    if (!d->db.open()) {
        Logger::instance().error(QStringLiteral("Cannot open database: %1").arg(d->db.lastError().text()),
                                 QStringLiteral("database"));
        return false;
    }

    d->db.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    d->db.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
    d->db.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));

    if (!createSchema(d->db) || !createIndexes(d->db)) {
        Logger::instance().error(QStringLiteral("Schema creation failed"), QStringLiteral("database"));
        d->db.close();
        return false;
    }

    d->open = true;
    Logger::instance().info(QStringLiteral("Database ready: %1").arg(d->path), QStringLiteral("database"));
    emit opened();
    return true;
}

bool DatabaseManager::isOpen() const
{
    return d->open;
}

QSqlDatabase DatabaseManager::connection() const
{
    return d->db;
}

QString DatabaseManager::path() const
{
    return d->path;
}

bool DatabaseManager::migrate()
{
    return createSchema(d->db) && createIndexes(d->db);
}

bool DatabaseManager::createSchema(QSqlDatabase& db) const
{
    QSqlQuery q(db);
    const QStringList statements = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS tasks ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "title TEXT NOT NULL,"
                       "notes TEXT DEFAULT '',"
                       "project_id INTEGER DEFAULT 0,"
                       "priority INTEGER DEFAULT 1,"
                       "status INTEGER DEFAULT 0,"
                       "due_at TEXT,"
                       "recurring TEXT DEFAULT '',"
                       "checklist TEXT DEFAULT '',"
                       "checked_items TEXT DEFAULT '',"
                       "sort_order INTEGER DEFAULT 0,"
                       "created_at TEXT,"
                       "completed_at TEXT)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS projects ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "name TEXT NOT NULL,"
                       "description TEXT DEFAULT '',"
                       "deadline TEXT,"
                       "archived INTEGER DEFAULT 0,"
                       "created_at TEXT)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS events ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "title TEXT NOT NULL,"
                       "notes TEXT DEFAULT '',"
                       "start_at TEXT,"
                       "end_at TEXT,"
                       "recurrence INTEGER DEFAULT 0,"
                       "all_day INTEGER DEFAULT 0,"
                       "task_id INTEGER DEFAULT 0,"
                       "created_at TEXT)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS journal ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "date TEXT NOT NULL UNIQUE,"
                       "reflection TEXT DEFAULT '',"
                       "gratitude TEXT DEFAULT '',"
                       "mistakes TEXT DEFAULT '',"
                       "lessons TEXT DEFAULT '',"
                       "scripture TEXT DEFAULT '',"
                       "tomorrow TEXT DEFAULT '',"
                       "updated_at TEXT)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS focus_sessions ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "kind INTEGER DEFAULT 0,"
                       "task_id INTEGER DEFAULT 0,"
                       "started_at TEXT,"
                       "duration_seconds INTEGER DEFAULT 0,"
                       "actual_seconds INTEGER DEFAULT 0,"
                       "note TEXT DEFAULT '')"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS habits ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "name TEXT NOT NULL,"
                       "target_per_week INTEGER DEFAULT 5,"
                       "created_at TEXT)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS habit_logs ("
                       "habit_id INTEGER NOT NULL,"
                       "date TEXT NOT NULL,"
                       "PRIMARY KEY (habit_id, date))"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS checkins ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "date TEXT NOT NULL UNIQUE,"
                       "mood INTEGER DEFAULT 0,"
                       "energy INTEGER DEFAULT 0,"
                       "note TEXT DEFAULT '')"),
    };
    for (const QString& sql : statements) {
        if (!q.exec(sql)) {
            Logger::instance().error(QStringLiteral("Schema error: %1").arg(q.lastError().text()),
                                     QStringLiteral("database"));
            return false;
        }
    }
    return true;
}

bool DatabaseManager::createIndexes(QSqlDatabase& db) const
{
    QSqlQuery q(db);
    const QStringList statements = {
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_tasks_due ON tasks(due_at)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_tasks_project ON tasks(project_id)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_events_start ON events(start_at)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_focus_started ON focus_sessions(started_at)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_focus_task ON focus_sessions(task_id)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_habit_logs_date ON habit_logs(date)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_checkins_date ON checkins(date)"),
    };
    for (const QString& sql : statements) {
        if (!q.exec(sql)) {
            Logger::instance().error(QStringLiteral("Index error: %1").arg(q.lastError().text()),
                                     QStringLiteral("database"));
            return false;
        }
    }
    return true;
}

} // namespace arete::data
