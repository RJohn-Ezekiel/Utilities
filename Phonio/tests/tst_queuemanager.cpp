#include <QtTest>

#include "queue/QueueManager.h"
#include "database/DatabaseManager.h"

#include <QTemporaryDir>

using namespace phonio;

class TestQueueManager : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void enqueueAndIndexOps();
    void enqueueNextInsertsAfterCurrent();
    void removeKeepsIndexConsistent();
    void moveOperations();
    void persistenceAcrossInstances();
    void trimRemovesMissingTracks();

private:
    QTemporaryDir* m_dir = nullptr;
    DatabaseManager* m_db = nullptr;
    QueueManager* m_queue = nullptr;
};

void TestQueueManager::init()
{
    m_dir = new QTemporaryDir;
    m_db = new DatabaseManager(m_dir->filePath(QStringLiteral("test.db")), this);
    m_queue = new QueueManager(m_db, this);
    m_queue->loadFromDatabase();

    for (int i = 0; i < 5; ++i) {
        Track track;
        track.filePath = QStringLiteral("/music/t%1.mp3").arg(i);
        m_db->upsertTrack(track);
    }
}

void TestQueueManager::enqueueAndIndexOps()
{
    const QVector<qint64> ids = {1, 2, 3};
    m_queue->enqueue(ids);
    QCOMPARE(m_queue->size(), 3);
    QCOMPARE(m_queue->trackIdAt(0), 1);

    m_queue->setCurrentIndex(1);
    QCOMPARE(m_queue->currentTrackId().value_or(-1), 2);

    m_queue->insertAt(0, 2);
    QCOMPARE(m_queue->size(), 4);
    QCOMPARE(m_queue->trackIdAt(0), 2);
    QCOMPARE(m_queue->currentTrackId().value_or(-1), 2); // shifted, still current
}

void TestQueueManager::enqueueNextInsertsAfterCurrent()
{
    m_queue->enqueue({1, 2, 3});
    m_queue->setCurrentIndex(0);
    m_queue->enqueueNext({8, 9});
    QCOMPARE(m_queue->trackIdAt(1), 8);
    QCOMPARE(m_queue->trackIdAt(2), 9);
    QCOMPARE(m_queue->trackIdAt(3), 2);
    QCOMPARE(m_queue->size(), 5);
}

void TestQueueManager::removeKeepsIndexConsistent()
{
    m_queue->enqueue({1, 2, 3, 4});
    m_queue->setCurrentIndex(2);
    m_queue->removeAt(0);
    QCOMPARE(m_queue->currentIndex(), 1); // shifted up
    QCOMPARE(m_queue->currentTrackId().value_or(-1), 3);

    m_queue->removeAt(1);
    QCOMPARE(m_queue->currentIndex(), -1); // removed the current track
}

void TestQueueManager::moveOperations()
{
    m_queue->enqueue({1, 2, 3});
    m_queue->move(0, 2);
    QCOMPARE(m_queue->trackIdAt(0), 2);
    QCOMPARE(m_queue->trackIdAt(1), 3);
    QCOMPARE(m_queue->trackIdAt(2), 1);

    m_queue->moveUp(1);                 // [2,3,1] -> [3,2,1]
    QCOMPARE(m_queue->trackIdAt(0), 3);
    QCOMPARE(m_queue->trackIdAt(1), 2);
    QCOMPARE(m_queue->trackIdAt(2), 1);

    m_queue->moveDown(0);               // [3,2,1] -> [2,3,1]
    QCOMPARE(m_queue->trackIdAt(0), 2);
    QCOMPARE(m_queue->trackIdAt(1), 3);
    QCOMPARE(m_queue->trackIdAt(2), 1);
}

void TestQueueManager::persistenceAcrossInstances()
{
    m_queue->enqueue({1, 2, 3, 4});
    m_queue->setCurrentIndex(2);

    QueueManager reloaded(m_db);
    reloaded.loadFromDatabase();
    QCOMPARE(reloaded.trackIds(), QVector<qint64>({1, 2, 3, 4}));
    QCOMPARE(reloaded.currentIndex(), -1); // current index is session state only
}

void TestQueueManager::trimRemovesMissingTracks()
{
    m_queue->enqueue({1, 2, 3, 4});
    m_queue->trimToExisting({2, 4});
    QCOMPARE(m_queue->trackIds(), QVector<qint64>({2, 4}));
    QCOMPARE(m_queue->size(), 2);
}

QTEST_MAIN(TestQueueManager)
#include "tst_queuemanager.moc"
