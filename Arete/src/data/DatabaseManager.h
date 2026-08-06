#pragma once

#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <memory>

namespace arete::data {

// Opens the SQLite database and owns the schema. All repositories share
// the single connection. Long-running work goes through QtConcurrent
// callers so the UI thread stays responsive.
class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(const QString& databasePath, QObject* parent = nullptr);
    ~DatabaseManager() override;

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    [[nodiscard]] bool open();
    [[nodiscard]] bool isOpen() const;

    [[nodiscard]] QSqlDatabase connection() const;
    [[nodiscard]] QString path() const;

    // Runs a lightweight migration; returns false and rolls back on failure.
    bool migrate();

signals:
    void opened();

private:
    bool createSchema(QSqlDatabase& db) const;
    bool createIndexes(QSqlDatabase& db) const;

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace arete::data
