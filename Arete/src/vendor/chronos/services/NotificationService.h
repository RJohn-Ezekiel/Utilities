#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QSystemTrayIcon;
QT_END_NAMESPACE

namespace chronos {

class AudioService;
class ToastNotification;

class NotificationService : public QObject {
    Q_OBJECT

public:
    explicit NotificationService(AudioService* audio, QObject* parent = nullptr);

    void showNotification(const QString& title, const QString& message);
    void showReminder(const QString& title, const QString& message);

signals:
    void inAppNotification(const QString& message);

private:
    void ensureTrayIcon();

    AudioService* m_audio = nullptr;
    QSystemTrayIcon* m_trayIcon = nullptr;
    ToastNotification* m_toast = nullptr;
};

} // namespace chronos
