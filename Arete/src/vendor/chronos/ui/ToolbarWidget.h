#pragma once

#include <QWidget>
#include <QPushButton>

#include "core/TimerState.h"

namespace chronos {

class ToolbarWidget : public QWidget {
    Q_OBJECT

public:
    explicit ToolbarWidget(QWidget* parent = nullptr);

    void updateForState(TimerState state);

signals:
    void startClicked();
    void pauseClicked();
    void resumeClicked();
    void stopClicked();
    void skipBreakClicked();
    void settingsClicked();
    void miniModeClicked();

private:
    QPushButton* m_startBtn = nullptr;
    QPushButton* m_pauseBtn = nullptr;
    QPushButton* m_resumeBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QPushButton* m_skipBreakBtn = nullptr;
    QPushButton* m_miniModeBtn = nullptr;
    QPushButton* m_settingsBtn = nullptr;
};

} // namespace chronos
