#pragma once

#include "core/Types.h"

#include <QObject>
#include <QVector>

namespace phonio {

class DatabaseManager;

// Ordered playback queue, persisted between launches.
// Holds track ids; resolves tracks through the library at the controller level.
class QueueManager : public QObject
{
    Q_OBJECT

public:
    explicit QueueManager(DatabaseManager* db, QObject* parent = nullptr);

    void loadFromDatabase();

    const QVector<qint64>& trackIds() const { return m_trackIds; }
    int size() const;
    bool isEmpty() const;
    int currentIndex() const { return m_currentIndex; }
    std::optional<qint64> currentTrackId() const;
    qint64 trackIdAt(int index) const;

    void enqueue(const QVector<qint64>& ids);
    void enqueueNext(const QVector<qint64>& ids);       // insert after current index
    void insertAt(int index, qint64 id);
    void removeAt(int index);
    void move(int fromIndex, int toIndex);
    void moveUp(int index);
    void moveDown(int index);
    void clear();
    void setCurrentIndex(int index);
    void setCurrentTrackId(qint64 id);

    // Guards the current index when tracks are removed externally.
    void trimToExisting(const QSet<qint64>& validIds);

signals:
    void queueChanged();

private:
    void persist();

    DatabaseManager* m_db;
    QVector<qint64> m_trackIds;
    int m_currentIndex = -1;
};

} // namespace phonio
