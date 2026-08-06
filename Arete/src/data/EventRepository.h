#pragma once

#include "models/CalendarEvent.h"
#include <QObject>
#include <QVector>

namespace arete::data {

class DatabaseManager;

class EventRepository : public QObject
{
    Q_OBJECT

public:
    explicit EventRepository(DatabaseManager* db, QObject* parent = nullptr);

    // Events whose start falls within the range (recurring events expanded).
    [[nodiscard]] QVector<models::CalendarEvent> between(const QDateTime& from, const QDateTime& to) const;
    [[nodiscard]] QVector<models::CalendarEvent> all() const;
    [[nodiscard]] models::CalendarEvent byId(qint64 id) const;

    qint64 insert(const models::CalendarEvent& event);
    bool update(const models::CalendarEvent& event);
    bool remove(qint64 id);

signals:
    void changed();

private:
    DatabaseManager* m_db;
};

} // namespace arete::data
