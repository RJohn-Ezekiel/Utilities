#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>

namespace arete::notifications {

enum class Type {
    Info,
    Success,
    Warning,
    Error,
    Progress
};

struct Notification
{
    QString id;
    Type type = Type::Info;
    QString title;
    QString message;
    QDateTime timestamp = QDateTime::currentDateTime();
    QVariantMap data;
    bool dismissible = true;
    bool persistent = false; // Persistent notifications don't auto-dismiss
    int timeoutMs = 5000;    // Auto-dismiss timeout (0 = no timeout)
    QString actionText;      // Optional action button text
    std::function<void()> actionCallback;
};

class NotificationModel;

class NotificationCenter : public QObject
{
    Q_OBJECT

public:
    static NotificationCenter& instance();

    // Non-copyable
    NotificationCenter(const NotificationCenter&) = delete;
    NotificationCenter& operator=(const NotificationCenter&) = delete;

    // Show notifications
    QString show(const Notification& notification);
    QString showInfo(const QString& title, const QString& message, int timeoutMs = 5000);
    QString showSuccess(const QString& title, const QString& message, int timeoutMs = 5000);
    QString showWarning(const QString& title, const QString& message, int timeoutMs = 8000);
    QString showError(const QString& title, const QString& message, int timeoutMs = 0); // Persistent by default
    QString showProgress(const QString& title, const QString& message, const QString& id = QString());

    // Update progress notification
    void updateProgress(const QString& id, int progress, const QString& message = QString());

    // Dismiss/clear
    void dismiss(const QString& id);
    void dismissAll();
    void dismissByType(Type type);

    // Model for UI
    [[nodiscard]] NotificationModel* model() const;

    // Settings
    void setDefaultTimeout(int timeoutMs);
    void setMaxVisible(int count);
    void setDoNotDisturb(bool enabled);
    [[nodiscard]] bool isDoNotDisturb() const;

    // Persistence
    [[nodiscard]] bool saveToFile(const QString& filePath) const;
    [[nodiscard]] bool loadFromFile(const QString& filePath);

signals:
    void notificationAdded(const Notification& notification);
    void notificationRemoved(const QString& id);
    void notificationUpdated(const Notification& notification);
    void cleared();
    void doNotDisturbChanged(bool enabled);

private:
    NotificationCenter();
    ~NotificationCenter() override;

    class Private;
    std::unique_ptr<Private> d;
};

class NotificationModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadCountChanged)

public:
    explicit NotificationModel(QObject* parent = nullptr);
    ~NotificationModel() override;

    enum Role {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        TitleRole,
        MessageRole,
        TimestampRole,
        DataRole,
        DismissibleRole,
        PersistentRole,
        TimeoutMsRole,
        ProgressRole,
        ActionTextRole
    };

    [[nodiscard]] int count() const;
    [[nodiscard]] int unreadCount() const;
    [[nodiscard]] Notification notificationAt(int index) const;
    [[nodiscard]] QVector<Notification> notifications() const;

    void addNotification(const Notification& notification);
    void removeNotification(const QString& id);
    void updateNotification(const Notification& notification);
    void clear();
    void markAsRead(const QString& id);

signals:
    void countChanged();
    void unreadCountChanged();
    void notificationsChanged();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace arete::notifications