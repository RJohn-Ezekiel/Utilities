#include <QTest>
#include <QSignalSpy>
#include <QTimer>

#include "core/TimerEngine.h"

using namespace chronos;

class TestTimerEngine : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitialState();
    void testStartFocus();
    void testPauseResume();
    void testStop();
    void testSessionComplete();
    void testSkipBreak();
    void testRestore();

private:
    TimerEngine* engine = nullptr;
};

void TestTimerEngine::initTestCase()
{
    engine = new TimerEngine(this);
}

void TestTimerEngine::testInitialState()
{
    QCOMPARE(engine->state(), TimerState::Idle);
    QCOMPARE(engine->remainingSeconds(), 0);
    QCOMPARE(engine->elapsedSeconds(), 0);
    QCOMPARE(engine->totalSeconds(), 0);
}

void TestTimerEngine::testStartFocus()
{
    QSignalSpy spyState(engine, &TimerEngine::stateChanged);
    QSignalSpy spyTick(engine, &TimerEngine::tick);

    engine->start(SessionType::Focus, 1500);

    QCOMPARE(engine->state(), TimerState::Focusing);
    QCOMPARE(engine->remainingSeconds(), 1500);
    QCOMPARE(engine->totalSeconds(), 1500);
    QCOMPARE(engine->elapsedSeconds(), 0);
    QCOMPARE(engine->currentSessionType(), SessionType::Focus);
    QCOMPARE(spyState.count(), 1);

    // Advance one tick
    QTest::qWait(1100);
    QCOMPARE(engine->remainingSeconds(), 1499);
    QCOMPARE(engine->elapsedSeconds(), 1);

    engine->stop();
}

void TestTimerEngine::testPauseResume()
{
    engine->start(SessionType::Focus, 600);

    QTest::qWait(1100);
    int remainingAfterTick = engine->remainingSeconds();

    engine->pause();
    QCOMPARE(engine->state(), TimerState::Paused);

    // Time should not advance while paused
    int remainingPaused = engine->remainingSeconds();
    QTest::qWait(500);
    QCOMPARE(engine->remainingSeconds(), remainingPaused);

    engine->resume();
    QCOMPARE(engine->state(), TimerState::Focusing);

    QTest::qWait(1100);
    QCOMPARE(engine->remainingSeconds(), remainingPaused - 1);

    engine->stop();
}

void TestTimerEngine::testStop()
{
    engine->start(SessionType::Focus, 1500);
    QTest::qWait(500);
    engine->stop();

    QCOMPARE(engine->state(), TimerState::Idle);
    QCOMPARE(engine->remainingSeconds(), 0);
    QCOMPARE(engine->elapsedSeconds(), 0);

    // Stop from pause
    engine->start(SessionType::ShortBreak, 300);
    QTest::qWait(500);
    engine->pause();
    engine->stop();

    QCOMPARE(engine->state(), TimerState::Idle);
}

void TestTimerEngine::testSessionComplete()
{
    QSignalSpy spyComplete(engine, &TimerEngine::sessionCompleted);

    engine->start(SessionType::Focus, 1);
    QTest::qWait(1100);

    QCOMPARE(spyComplete.count(), 1);
    QCOMPARE(spyComplete.at(0).at(0).value<SessionType>(), SessionType::Focus);
    QCOMPARE(engine->remainingSeconds(), 0);

    engine->stop(); // reset
}

void TestTimerEngine::testSkipBreak()
{
    QSignalSpy spyComplete(engine, &TimerEngine::sessionCompleted);

    engine->start(SessionType::ShortBreak, 300);
    engine->skipBreak();

    QCOMPARE(spyComplete.count(), 1);
    QCOMPARE(engine->state(), TimerState::Idle);

    // Should not skip from focusing
    engine->start(SessionType::Focus, 1500);
    engine->skipBreak(); // no-op for focus
    QCOMPARE(engine->state(), TimerState::Focusing);

    engine->stop();
}

void TestTimerEngine::testRestore()
{
    engine->restore(TimerState::Focusing, SessionType::Focus, 1200, 1500);

    QCOMPARE(engine->state(), TimerState::Focusing);
    QCOMPARE(engine->remainingSeconds(), 1200);
    QCOMPARE(engine->elapsedSeconds(), 300);
    QCOMPARE(engine->totalSeconds(), 1500);

    // Timer should be running
    QTest::qWait(1100);
    QCOMPARE(engine->remainingSeconds(), 1199);

    engine->stop();

    // Restore to paused
    engine->restore(TimerState::Paused, SessionType::Focus, 800, 1500);
    QCOMPARE(engine->state(), TimerState::Paused);

    // Time should not advance
    QTest::qWait(500);
    QCOMPARE(engine->remainingSeconds(), 800);

    engine->resume();
    QTest::qWait(1100);
    QCOMPARE(engine->remainingSeconds(), 799);

    engine->stop();
}

QTEST_MAIN(TestTimerEngine)
#include "test_timer_engine.moc"
