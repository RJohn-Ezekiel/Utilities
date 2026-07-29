#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QList>

namespace chronos {

class ToastNotification : public QWidget {
    Q_OBJECT

public:
    explicit ToastNotification(QWidget* parent = nullptr);

    void showMessage(const QString& title, const QString& message,
                     int timeoutMs = 6000);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void positionOnScreen();
    void fadeOut();

    QLabel* m_titleLabel = nullptr;
    QLabel* m_messageLabel = nullptr;
    QPushButton* m_closeBtn = nullptr;
    QTimer* m_timer = nullptr;
    QTimer* m_fadeTimer = nullptr;
    qreal m_opacity = 1.0;
};

} // namespace chronos
