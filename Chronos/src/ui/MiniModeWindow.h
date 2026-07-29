#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>

#include "core/TimerEngine.h"

namespace chronos {

class TimerService;
class CircularTimerWidget;

class MiniModeWindow : public QWidget {
    Q_OBJECT

public:
    explicit MiniModeWindow(TimerService* timerService, QWidget* parent = nullptr);

    void updateDisplay(int remainingSeconds, int elapsedSeconds, int totalSeconds);
    void updateState(TimerState state, const QString& taskLabel);

signals:
    void pauseClicked();
    void resumeClicked();
    void restoreClicked();

private:
    void setupUi();
    void applyStyle();

    TimerService* m_timerService = nullptr;
    CircularTimerWidget* m_circularTimer = nullptr;
    QPushButton* m_pauseBtn = nullptr;
    QPushButton* m_resumeBtn = nullptr;
    QLabel* m_taskLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
};

} // namespace chronos
