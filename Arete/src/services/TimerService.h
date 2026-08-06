#pragma once

#include <QObject>
#include <QString>

namespace arete {
namespace app { class AppContext; }
namespace services {

enum class TimerMode { Idle, Focus, ShortBreak, LongBreak };

// Arete's timer facade over the vendored Chronos engine. Keeps the shell,
// Home dashboard, top bar and command palette decoupled from the engine.
class TimerService : public QObject
{
    Q_OBJECT

public:
    explicit TimerService(app::AppContext* context, QObject* parent = nullptr);

    [[nodiscard]] TimerMode mode() const;
    [[nodiscard]] bool isRunning() const;
    [[nodiscard]] int remainingSeconds() const;
    [[nodiscard]] int elapsedSeconds() const;
    [[nodiscard]] int totalSeconds() const;
    [[nodiscard]] qint64 attachedTaskId() const;
    [[nodiscard]] QString attachedTaskTitle() const;

    // Statistics.
    [[nodiscard]] int focusMinutesToday() const;
    [[nodiscard]] int focusStreak() const;
    [[nodiscard]] int sessionsToday() const;
    [[nodiscard]] int weeklyFocusMinutes() const;

public slots:
    void startFocus(qint64 taskId = 0, const QString& taskTitle = {});
    void startShortBreak();
    void startLongBreak();
    // Custom-length focus session with a free-form label (workday flows).
    void startCustomTimer(int minutes, const QString& label);
    void pause();
    void resume();
    void stop();

signals:
    void stateChanged();
    void tick(int remainingSeconds, int elapsedSeconds);

private:
    void syncFromEngine();

    app::AppContext* m_context;
};

} // namespace services
} // namespace arete
