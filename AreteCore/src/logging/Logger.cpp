#include "arete/logging/Logger.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <mutex>
#include <algorithm>

namespace arete::logging {

class Logger::Private
{
public:
    Private()
        : minLevel(Level::Info)
        , logToFileEnabled(false)
        , logToConsoleEnabled(false)
        , maxFileSize(10 * 1024 * 1024) // 10 MB
        , maxFileCount(5)
    {
        // Default log file path
        QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (logDir.isEmpty()) {
            logDir = QDir::homePath() + "/.local/share/arete";
        }
        QDir().mkpath(logDir);
        logFilePath = logDir + "/arete.log";
    }

    Level minLevel;
    bool logToFileEnabled;
    bool logToConsoleEnabled;
    QString logFilePath;
    qint64 maxFileSize;
    int maxFileCount;
    std::unique_ptr<LogModel> model;
    std::unordered_map<QString, bool> categoryFilters;
    std::mutex mutex;
    QFile logFile;
    QTextStream logStream;
};

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
    : QObject(nullptr), d(std::make_unique<Private>())
{
    d->model = std::make_unique<LogModel>(this);
    openLogFile();
}

Logger::~Logger()
{
    closeLogFile();
}

void Logger::openLogFile()
{
    if (!d->logToFileEnabled) return;

    closeLogFile();

    QDir dir = QFileInfo(d->logFilePath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    d->logFile.setFileName(d->logFilePath);
    if (d->logFile.open(QIODevice::Append | QIODevice::Text)) {
        d->logStream.setDevice(&d->logFile);
    }
}

void Logger::closeLogFile()
{
    if (d->logFile.isOpen()) {
        d->logStream.flush();
        d->logFile.close();
    }
}

void Logger::rotateLogFile()
{
    if (!d->logFile.isOpen()) return;

    if (d->logFile.size() >= d->maxFileSize) {
        closeLogFile();

        // Rotate files
        for (int i = d->maxFileCount - 1; i >= 1; --i) {
            QString oldFile = d->logFilePath + "." + QString::number(i);
            QString newFile = d->logFilePath + "." + QString::number(i + 1);
            if (QFile::exists(oldFile)) {
                if (i == d->maxFileCount - 1) {
                    QFile::remove(newFile);
                }
                QFile::rename(oldFile, newFile);
            }
        }

        QFile::rename(d->logFilePath, d->logFilePath + ".1");
        openLogFile();
    }
}

void Logger::setMinLevel(Level level)
{
    d->minLevel = level;
}

Level Logger::minLevel() const
{
    return d->minLevel;
}

void Logger::setLogToFile(bool enabled)
{
    if (d->logToFileEnabled != enabled) {
        d->logToFileEnabled = enabled;
        if (enabled) {
            openLogFile();
        } else {
            closeLogFile();
        }
    }
}

bool Logger::logToFile() const
{
    return d->logToFileEnabled;
}

void Logger::setLogFilePath(const QString& path)
{
    d->logFilePath = path;
    if (d->logToFileEnabled) {
        openLogFile();
    }
}

QString Logger::logFilePath() const
{
    return d->logFilePath;
}

void Logger::setMaxFileSize(qint64 bytes)
{
    d->maxFileSize = bytes;
}

void Logger::setMaxFileCount(int count)
{
    d->maxFileCount = std::max(1, count);
}

void Logger::setLogToConsole(bool enabled)
{
    d->logToConsoleEnabled = enabled;
}

bool Logger::logToConsole() const
{
    return d->logToConsoleEnabled;
}

void Logger::log(Level level, const QString& message, const QString& category,
                 const QVariantMap& context, const char* file, int line, const char* function)
{
    if (level < d->minLevel) return;

    // Check category filter
    if (!category.isEmpty()) {
        auto it = d->categoryFilters.find(category);
        if (it != d->categoryFilters.end() && !it->second) {
            return;
        }
    }

    LogEntry entry;
    entry.level = level;
    entry.message = message;
    entry.category = category;
    entry.timestamp = QDateTime::currentDateTime();
    entry.context = context;
    if (file) entry.file = QString::fromUtf8(file);
    entry.line = line;
    if (function) entry.function = QString::fromUtf8(function);

    // Add to model
    d->model->addEntry(entry);

    // Write to file
    if (d->logToFileEnabled) {
        std::lock_guard<std::mutex> lock(d->mutex);
        rotateLogFile();
        if (d->logFile.isOpen()) {
            QString levelStr;
            switch (level) {
                case Level::Trace: levelStr = "TRACE"; break;
                case Level::Debug: levelStr = "DEBUG"; break;
                case Level::Info: levelStr = "INFO"; break;
                case Level::Warning: levelStr = "WARN"; break;
                case Level::Error: levelStr = "ERROR"; break;
                case Level::Critical: levelStr = "CRIT"; break;
            }
            d->logStream << entry.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz")
                         << " [" << levelStr << "]"
                         << (category.isEmpty() ? "" : " [" + category + "]")
                         << " " << message;
            if (!context.isEmpty()) {
                QJsonObject ctxObj;
                for (auto it = context.begin(); it != context.end(); ++it) {
                    ctxObj[it.key()] = QJsonValue::fromVariant(it.value());
                }
                d->logStream << " " << QJsonDocument(ctxObj).toJson(QJsonDocument::Compact);
            }
            d->logStream << "\n";
            d->logStream.flush();
        }
    }

    // Console output (only for warnings/errors in production, or all in debug)
    if (d->logToConsoleEnabled) {
        QString levelStr;
        switch (level) {
            case Level::Trace: levelStr = "TRACE"; break;
            case Level::Debug: levelStr = "DEBUG"; break;
            case Level::Info: levelStr = "INFO"; break;
            case Level::Warning: levelStr = "WARN"; break;
            case Level::Error: levelStr = "ERROR"; break;
            case Level::Critical: levelStr = "CRIT"; break;
        }
        fprintf(stderr, "%s [%s] %s\n",
                entry.timestamp.toString("hh:mm:ss.zzz").toUtf8().constData(),
                levelStr.toUtf8().constData(),
                message.toUtf8().constData());
    }

    emit entryAdded(entry);
}

void Logger::trace(const QString& message, const QString& category,
                   const QVariantMap& context, const char* file, int line, const char* function)
{
    log(Level::Trace, message, category, context, file, line, function);
}

void Logger::debug(const QString& message, const QString& category,
                   const QVariantMap& context, const char* file, int line, const char* function)
{
    log(Level::Debug, message, category, context, file, line, function);
}

void Logger::info(const QString& message, const QString& category,
                  const QVariantMap& context, const char* file, int line, const char* function)
{
    log(Level::Info, message, category, context, file, line, function);
}

void Logger::warning(const QString& message, const QString& category,
                     const QVariantMap& context, const char* file, int line, const char* function)
{
    log(Level::Warning, message, category, context, file, line, function);
}

void Logger::error(const QString& message, const QString& category,
                   const QVariantMap& context, const char* file, int line, const char* function)
{
    log(Level::Error, message, category, context, file, line, function);
}

void Logger::critical(const QString& message, const QString& category,
                      const QVariantMap& context, const char* file, int line, const char* function)
{
    log(Level::Critical, message, category, context, file, line, function);
}

LogModel* Logger::model() const
{
    return d->model.get();
}

void Logger::setCategoryFilter(const QString& category, bool enabled)
{
    d->categoryFilters[category] = enabled;
    emit categoryFilterChanged(category, enabled);
}

bool Logger::isCategoryEnabled(const QString& category) const
{
    auto it = d->categoryFilters.find(category);
    if (it != d->categoryFilters.end()) {
        return it->second;
    }
    return true; // Default enabled
}

void Logger::clear()
{
    d->model->clear();
}

bool Logger::exportToFile(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonArray array;
    for (const LogEntry& entry : d->model->entries()) {
        QJsonObject obj;
        obj["level"] = static_cast<int>(entry.level);
        obj["message"] = entry.message;
        obj["category"] = entry.category;
        obj["timestamp"] = entry.timestamp.toString(Qt::ISODateWithMs);
        QJsonObject ctx;
        for (auto it = entry.context.begin(); it != entry.context.end(); ++it) {
            ctx[it.key()] = QJsonValue::fromVariant(it.value());
        }
        obj["context"] = ctx;
        obj["file"] = entry.file;
        obj["line"] = entry.line;
        obj["function"] = entry.function;
        array.append(obj);
    }

    QJsonDocument doc(array);
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

// ============================================================================
// LogModel Implementation
// ============================================================================

class LogModel::Private
{
public:
    QVector<LogEntry> entries;
    std::mutex mutex;
};

LogModel::~LogModel() = default;

LogModel::LogModel(QObject* parent)
    : QObject(parent), d(std::make_unique<Private>())
{
}



int LogModel::count() const
{
    std::lock_guard<std::mutex> lock(d->mutex);
    return d->entries.size();
}

LogEntry LogModel::entryAt(int index) const
{
    std::lock_guard<std::mutex> lock(d->mutex);
    if (index >= 0 && index < d->entries.size()) {
        return d->entries[index];
    }
    return LogEntry{};
}

QVector<LogEntry> LogModel::entries() const
{
    std::lock_guard<std::mutex> lock(d->mutex);
    return d->entries;
}

void LogModel::addEntry(const LogEntry& entry)
{
    std::lock_guard<std::mutex> lock(d->mutex);
    d->entries.append(entry);
    emit countChanged();
    emit entriesChanged();
}

void LogModel::clear()
{
    std::lock_guard<std::mutex> lock(d->mutex);
    d->entries.clear();
    emit countChanged();
    emit entriesChanged();
}

} // namespace arete::logging