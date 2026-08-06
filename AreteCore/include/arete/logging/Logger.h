#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVariant>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>

namespace arete::logging {

enum class Level {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Critical = 5
};

struct LogEntry
{
    Level level = Level::Info;
    QString message;
    QString category;
    QDateTime timestamp = QDateTime::currentDateTime();
    QVariantMap context;
    QString file;
    int line = 0;
    QString function;
};

class LogModel;

class Logger : public QObject
{
    Q_OBJECT

public:
    static Logger& instance();

    // Pointer access for connection safety
    static Logger* instancePtr() { return &instance(); }

    // Non-copyable
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Configuration
    void setMinLevel(Level level);
    [[nodiscard]] Level minLevel() const;

    void setLogToFile(bool enabled);
    [[nodiscard]] bool logToFile() const;

    void setLogFilePath(const QString& path);
    [[nodiscard]] QString logFilePath() const;

    void setMaxFileSize(qint64 bytes);
    void setMaxFileCount(int count);

    void setLogToConsole(bool enabled);
    [[nodiscard]] bool logToConsole() const;

    // Logging
    void log(Level level, const QString& message, const QString& category = QString(),
             const QVariantMap& context = {}, const char* file = nullptr,
             int line = 0, const char* function = nullptr);

    // Convenience methods
    void trace(const QString& message, const QString& category = QString(),
               const QVariantMap& context = {}, const char* file = nullptr,
               int line = 0, const char* function = nullptr);
    void debug(const QString& message, const QString& category = QString(),
               const QVariantMap& context = {}, const char* file = nullptr,
               int line = 0, const char* function = nullptr);
    void info(const QString& message, const QString& category = QString(),
              const QVariantMap& context = {}, const char* file = nullptr,
              int line = 0, const char* function = nullptr);
    void warning(const QString& message, const QString& category = QString(),
                 const QVariantMap& context = {}, const char* file = nullptr,
                 int line = 0, const char* function = nullptr);
    void error(const QString& message, const QString& category = QString(),
               const QVariantMap& context = {}, const char* file = nullptr,
               int line = 0, const char* function = nullptr);
    void critical(const QString& message, const QString& category = QString(),
                  const QVariantMap& context = {}, const char* file = nullptr,
                  int line = 0, const char* function = nullptr);

    // Log model for UI
    [[nodiscard]] LogModel* model() const;

    // Filtering
    void setCategoryFilter(const QString& category, bool enabled);
    [[nodiscard]] bool isCategoryEnabled(const QString& category) const;

    // Clear
    void clear();

    // Export
    [[nodiscard]] bool exportToFile(const QString& filePath) const;

signals:
    void entryAdded(const LogEntry& entry);
    void cleared();
    void categoryFilterChanged(const QString& category, bool enabled);

private:
    Logger();
    ~Logger() override;

    void openLogFile();
    void closeLogFile();
    void rotateLogFile();

    class Private;
    std::unique_ptr<Private> d;
};

// Log model for views
class LogModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit LogModel(QObject* parent = nullptr);
    ~LogModel() override;

    enum Role {
        LevelRole = Qt::UserRole + 1,
        MessageRole,
        CategoryRole,
        TimestampRole,
        ContextRole,
        FileRole,
        LineRole,
        FunctionRole
    };

    [[nodiscard]] int count() const;
    [[nodiscard]] LogEntry entryAt(int index) const;
    [[nodiscard]] QVector<LogEntry> entries() const;

    void addEntry(const LogEntry& entry);
    void clear();

signals:
    void countChanged();
    void entriesChanged();

private:
    class Private;
    std::unique_ptr<Private> d;
};

// Macros for automatic file/line/function capture
#define ARETE_LOG_TRACE(logger, msg, ...) \
    logger.trace(msg, ##__VA_ARGS__, __FILE__, __LINE__, __func__)

#define ARETE_LOG_DEBUG(logger, msg, ...) \
    logger.debug(msg, ##__VA_ARGS__, __FILE__, __LINE__, __func__)

#define ARETE_LOG_INFO(logger, msg, ...) \
    logger.info(msg, ##__VA_ARGS__, __FILE__, __LINE__, __func__)

#define ARETE_LOG_WARNING(logger, msg, ...) \
    logger.warning(msg, ##__VA_ARGS__, __FILE__, __LINE__, __func__)

#define ARETE_LOG_ERROR(logger, msg, ...) \
    logger.error(msg, ##__VA_ARGS__, __FILE__, __LINE__, __func__)

#define ARETE_LOG_CRITICAL(logger, msg, ...) \
    logger.critical(msg, ##__VA_ARGS__, __FILE__, __LINE__, __func__)

// Global logger access
#define ARETE_LOG(level, msg, ...) \
    arete::logging::Logger::instance().level(msg, ##__VA_ARGS__)

} // namespace arete::logging