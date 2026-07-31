#include "queue/QueueManager.h"

#include "database/DatabaseManager.h"

#include <algorithm>
#include <QSet>

namespace phonio {

QueueManager::QueueManager(DatabaseManager* db, QObject* parent)
    : QObject(parent)
    , m_db(db)
{
}

void QueueManager::loadFromDatabase()
{
    m_trackIds = m_db->loadQueue();
    emit queueChanged();
}

int QueueManager::size() const
{
    return m_trackIds.size();
}

bool QueueManager::isEmpty() const
{
    return m_trackIds.isEmpty();
}

std::optional<qint64> QueueManager::currentTrackId() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_trackIds.size())
        return std::nullopt;
    return m_trackIds.at(m_currentIndex);
}

qint64 QueueManager::trackIdAt(int index) const
{
    return (index >= 0 && index < m_trackIds.size()) ? m_trackIds.at(index) : -1;
}

void QueueManager::enqueue(const QVector<qint64>& ids)
{
    m_trackIds += ids;
    persist();
}

void QueueManager::enqueueNext(const QVector<qint64>& ids)
{
    int insertPos = (m_currentIndex >= 0 && m_currentIndex < m_trackIds.size()) ? m_currentIndex + 1 : m_trackIds.size();
    for (int i = ids.size() - 1; i >= 0; --i)
        m_trackIds.insert(insertPos, ids.at(i));
    persist();
}

void QueueManager::insertAt(int index, qint64 id)
{
    m_trackIds.insert(qBound(0, index, m_trackIds.size()), id);
    if (m_currentIndex >= index && m_currentIndex >= 0)
        ++m_currentIndex;
    persist();
}

void QueueManager::removeAt(int index)
{
    if (index < 0 || index >= m_trackIds.size())
        return;
    m_trackIds.removeAt(index);
    if (index < m_currentIndex)
        --m_currentIndex;
    else if (index == m_currentIndex)
        m_currentIndex = -1;
    persist();
}

void QueueManager::move(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= m_trackIds.size())
        return;
    toIndex = qBound(0, toIndex, m_trackIds.size() - 1);
    if (fromIndex == toIndex)
        return;
    const qint64 id = m_trackIds.takeAt(fromIndex);
    m_trackIds.insert(toIndex, id);
    if (m_currentIndex == fromIndex)
        m_currentIndex = toIndex;
    else if (fromIndex < m_currentIndex && toIndex >= m_currentIndex)
        --m_currentIndex;
    else if (fromIndex > m_currentIndex && toIndex <= m_currentIndex)
        ++m_currentIndex;
    persist();
}

void QueueManager::moveUp(int index)
{
    move(index, index - 1);
}

void QueueManager::moveDown(int index)
{
    move(index, index + 1);
}

void QueueManager::clear()
{
    m_trackIds.clear();
    m_currentIndex = -1;
    persist();
}

void QueueManager::setCurrentIndex(int index)
{
    if (index < -1 || index >= m_trackIds.size())
        return;
    m_currentIndex = index;
    persist();
}

void QueueManager::setCurrentTrackId(qint64 id)
{
    const int index = m_trackIds.indexOf(id);
    if (index >= 0)
        m_currentIndex = index;
}

void QueueManager::trimToExisting(const QSet<qint64>& validIds)
{
    QVector<qint64> filtered;
    filtered.reserve(m_trackIds.size());
    for (qint64 id : m_trackIds) {
        if (validIds.contains(id))
            filtered.append(id);
    }
    if (filtered.size() != m_trackIds.size()) {
        m_trackIds = filtered;
        m_currentIndex = -1;
        persist();
    }
}

void QueueManager::persist()
{
    m_db->saveQueue(m_trackIds);
    emit queueChanged();
}

} // namespace phonio
