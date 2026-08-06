#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <memory>

namespace arete::notifications {
class NotificationCenter;
}

namespace arete::widgets {

// A panel showing the application's notifications.
// Notifications are dismissible and clearable.
class NotificationsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit NotificationsPanel(QWidget* parent = nullptr);
    ~NotificationsPanel() override;

    void setNotificationCenter(notifications::NotificationCenter* center);

    // Counts
    [[nodiscard]] int notificationCount() const;
    [[nodiscard]] int unreadCount() const;

    // Actions
    void dismiss(const QString& id);
    void clearAll();

    // Filter buttons
    void setTypeFilter(int index); // 0=All, 1=Info, 2=Success, 3=Warning, 4=Error, 5=Progress

signals:
    void panelCleared();
    void notificationDismissed(const QString& id);
    void actionRequested(const QString& id);

private:
    void refresh();

    class Private;
    std::unique_ptr<Private> d;
};

} // namespace arete::widgets