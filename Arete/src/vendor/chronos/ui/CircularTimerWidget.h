#pragma once

#include <QWidget>
#include <QString>

namespace chronos {

class CircularTimerWidget : public QWidget {
    Q_OBJECT

public:
    explicit CircularTimerWidget(QWidget* parent = nullptr);

    void setProgress(int remainingSeconds, int totalSeconds);
    void setSessionLabel(const QString& label);
    void setTaskLabel(const QString& label);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

private:
    int m_remainingSeconds = 0;
    int m_totalSeconds = 0;
    QString m_sessionLabel;
    QString m_taskLabel;
};

} // namespace chronos
