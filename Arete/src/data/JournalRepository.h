#pragma once

#include "models/JournalEntry.h"
#include <QObject>
#include <QVector>

namespace arete::data {

class DatabaseManager;

class JournalRepository : public QObject
{
    Q_OBJECT

public:
    explicit JournalRepository(DatabaseManager* db, QObject* parent = nullptr);

    [[nodiscard]] models::JournalEntry forDate(const QDate& date) const;
    [[nodiscard]] QVector<models::JournalEntry> recent(int limit = 30) const;
    [[nodiscard]] QVector<models::JournalEntry> search(const QString& term) const;

    qint64 upsert(const models::JournalEntry& entry);
    bool remove(const QDate& date);

    [[nodiscard]] int streak() const;

signals:
    void changed();

private:
    DatabaseManager* m_db;
};

} // namespace arete::data
