#pragma once

#include <QWidget>
#include <QLabel>

#include "core/TimerState.h"

namespace chronos {

class StatusBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatusBarWidget(QWidget* parent = nullptr);

    void setStateText(const QString& text);
    void setNotification(const QString& text, int timeoutMs = 0);
    void updateForState(TimerState state);

private:
    QLabel* m_stateLabel = nullptr;
    QLabel* m_notificationLabel = nullptr;
    QLabel* m_shortcutLabel = nullptr;
};

} // namespace chronos
