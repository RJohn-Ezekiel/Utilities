#include "NotificationService.h"
#include "AudioService.h"
#include "ui/ToastNotification.h"

#include <QSystemTrayIcon>
#include <QPixmap>
#include <QPainter>

namespace chronos {

NotificationService::NotificationService(AudioService* audio, QObject* parent)
    : QObject(parent)
    , m_audio(audio)
{
    ensureTrayIcon();

    // Toast popup for screen notifications (works even when minimized)
    m_toast = new ToastNotification();
}

void NotificationService::showNotification(const QString& title,
                                           const QString& message)
{
    emit inAppNotification(message);
    m_audio->playNotification();

    if (m_trayIcon && m_trayIcon->supportsMessages()) {
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 8000);
    }

    // Always show on-screen toast popup
    m_toast->showMessage(title, message);
}

void NotificationService::showReminder(const QString& title,
                                       const QString& message)
{
    showNotification(title, message);
}

void NotificationService::ensureTrayIcon()
{
    if (m_trayIcon) return;

    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    {
        QPainter p(&pixmap);
        p.setBrush(QColor(122, 138, 154));
        p.setPen(Qt::NoPen);
        p.drawEllipse(1, 1, 14, 14);
    }

    m_trayIcon = new QSystemTrayIcon(QIcon(pixmap), this);
    m_trayIcon->setToolTip(QStringLiteral("Chronos"));
    m_trayIcon->show();
}

} // namespace chronos
