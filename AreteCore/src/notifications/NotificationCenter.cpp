#include "arete/notifications/NotificationCenter.h"
#include <QUuid>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QProcess>
#include <QDBusConnection>
#include <QDBusInterface>
#include <algorithm>
#include <mutex>

namespace arete::notifications {

namespace {

// Mirrors every notification to the desktop notification daemon (DBus),
// falling back to notify-send when there is no session bus. Gated by the
// "notifications/system" setting, so it can be switched off in Settings.
void forwardToSystem(const Notification& notification)
{
    if (notification.type == Type::Progress) return;

    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QStringLiteral("Arete"), QStringLiteral("Arete"));
    if (!settings.value(QStringLiteral("notifications/system"), true).toBool()) return;

    const QString summary = notification.title;
    const QString body = notification.message;
    const int timeout = notification.timeoutMs > 0 ? notification.timeoutMs : 0;

    if (QDBusConnection::sessionBus().isConnected()) {
        QDBusInterface iface(QStringLiteral("org.freedesktop.Notifications"),
                             QStringLiteral("/org/freedesktop/Notifications"),
                             QStringLiteral("org.freedesktop.Notifications"),
                             QDBusConnection::sessionBus());
        if (iface.isValid()) {
            iface.asyncCall(QStringLiteral("Notify"),
                            QStringLiteral("Arete"), 0u, QString(),
                            summary, body, QStringList(), QVariantMap(), timeout);
            return;
        }
    }
    QProcess::startDetached(QStringLiteral("notify-send"),
                            {QStringLiteral("-a"), QStringLiteral("Arete"),
                             QStringLiteral("-t"), QString::number(timeout),
                             summary, body});
}

} // namespace

class NotificationCenter::Private
{
public:
    Private()
        : defaultTimeout(5000)
        , maxVisible(10)
        , doNotDisturb(false)
    {
        model = std::make_unique<NotificationModel>();
    }

    int defaultTimeout;
    int maxVisible;
    bool doNotDisturb;
    std::unique_ptr<NotificationModel> model;
    std::unordered_map<QString, QTimer*> progressTimers;
    std::mutex mutex;
};

NotificationCenter& NotificationCenter::instance()
{
    static NotificationCenter instance;
    return instance;
}

NotificationCenter::NotificationCenter()
    : QObject(nullptr), d(std::make_unique<Private>())
{
}

NotificationCenter::~NotificationCenter()
{
    for (auto& [id, timer] : d->progressTimers) {
        timer->stop();
        timer->deleteLater();
    }
}

QString NotificationCenter::show(const Notification& notification)
{
    if (d->doNotDisturb && notification.type != Type::Error) {
        return QString();
    }

    Notification n = notification;
    if (n.id.isEmpty()) {
        n.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // Limit visible notifications
    {
        std::lock_guard<std::mutex> lock(d->mutex);
        auto notifications = d->model->notifications();
        while (notifications.size() >= static_cast<size_t>(d->maxVisible)) {
            // Remove oldest dismissible non-persistent notification
            auto it = std::find_if(notifications.begin(), notifications.end(),
                [](const Notification& n) { return n.dismissible && !n.persistent; });
            if (it != notifications.end()) {
                d->model->removeNotification(it->id);
                notifications = d->model->notifications();
            } else {
                break;
            }
        }
    }

    d->model->addNotification(n);

    // Auto-dismiss timer
    if (n.timeoutMs > 0 && !n.persistent) {
        QTimer::singleShot(n.timeoutMs, this, [this, id = n.id]() {
            dismiss(id);
        });
    }

    forwardToSystem(n);
    emit notificationAdded(n);
    return n.id;
}

QString NotificationCenter::showInfo(const QString& title, const QString& message, int timeoutMs)
{
    Notification n;
    n.type = Type::Info;
    n.title = title;
    n.message = message;
    n.timeoutMs = timeoutMs;
    return show(n);
}

QString NotificationCenter::showSuccess(const QString& title, const QString& message, int timeoutMs)
{
    Notification n;
    n.type = Type::Success;
    n.title = title;
    n.message = message;
    n.timeoutMs = timeoutMs;
    return show(n);
}

QString NotificationCenter::showWarning(const QString& title, const QString& message, int timeoutMs)
{
    Notification n;
    n.type = Type::Warning;
    n.title = title;
    n.message = message;
    n.timeoutMs = timeoutMs;
    return show(n);
}

QString NotificationCenter::showError(const QString& title, const QString& message, int timeoutMs)
{
    Notification n;
    n.type = Type::Error;
    n.title = title;
    n.message = message;
    n.timeoutMs = timeoutMs > 0 ? timeoutMs : 0; // Persistent by default
    n.persistent = timeoutMs <= 0;
    return show(n);
}

QString NotificationCenter::showProgress(const QString& title, const QString& message, const QString& id)
{
    Notification n;
    n.type = Type::Progress;
    n.title = title;
    n.message = message;
    n.id = id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id;
    n.persistent = true;
    n.dismissible = false;
    n.timeoutMs = 0;
    n.data["progress"] = 0;
    return show(n);
}

void NotificationCenter::updateProgress(const QString& id, int progress, const QString& message)
{
    auto notifications = d->model->notifications();
    auto it = std::find_if(notifications.begin(), notifications.end(),
        [&id](const Notification& n) { return n.id == id; });

    if (it != notifications.end()) {
        Notification n = *it;
        n.data["progress"] = std::clamp(progress, 0, 100);
        if (!message.isEmpty()) {
            n.message = message;
        }
        d->model->updateNotification(n);
        emit notificationUpdated(n);
    }
}

void NotificationCenter::dismiss(const QString& id)
{
    d->model->removeNotification(id);
    emit notificationRemoved(id);
}

void NotificationCenter::dismissAll()
{
    d->model->clear();
    emit cleared();
}

void NotificationCenter::dismissByType(Type type)
{
    auto notifications = d->model->notifications();
    for (const auto& n : notifications) {
        if (n.type == type && n.dismissible) {
            d->model->removeNotification(n.id);
            emit notificationRemoved(n.id);
        }
    }
}

NotificationModel* NotificationCenter::model() const
{
    return d->model.get();
}

void NotificationCenter::setDefaultTimeout(int timeoutMs)
{
    d->defaultTimeout = std::max(0, timeoutMs);
}

void NotificationCenter::setMaxVisible(int count)
{
    d->maxVisible = std::max(1, count);
}

void NotificationCenter::setDoNotDisturb(bool enabled)
{
    if (d->doNotDisturb != enabled) {
        d->doNotDisturb = enabled;
        emit doNotDisturbChanged(enabled);
    }
}

bool NotificationCenter::isDoNotDisturb() const
{
    return d->doNotDisturb;
}

bool NotificationCenter::saveToFile(const QString& filePath) const
{
    QJsonArray array;
    for (const auto& n : d->model->notifications()) {
        if (!n.persistent) continue; // Only save persistent notifications

        QJsonObject obj;
        obj["id"] = n.id;
        obj["type"] = static_cast<int>(n.type);
        obj["title"] = n.title;
        obj["message"] = n.message;
        obj["timestamp"] = n.timestamp.toString(Qt::ISODateWithMs);
        QJsonObject dataObj;
        for (auto it = n.data.begin(); it != n.data.end(); ++it) {
            dataObj[it.key()] = QJsonValue::fromVariant(it.value());
        }
        obj["data"] = dataObj;
        obj["dismissible"] = n.dismissible;
        obj["persistent"] = n.persistent;
        obj["timeoutMs"] = n.timeoutMs;
        obj["actionText"] = n.actionText;
        array.append(obj);
    }

    QJsonDocument doc(array);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool NotificationCenter::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isArray()) {
        return false;
    }

    QJsonArray array = doc.array();
    for (const QJsonValue& val : array) {
        QJsonObject obj = val.toObject();
        Notification n;
        n.id = obj["id"].toString();
        n.type = static_cast<Type>(obj["type"].toInt());
        n.title = obj["title"].toString();
        n.message = obj["message"].toString();
        n.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODateWithMs);
        QJsonObject dataObj = obj["data"].toObject();
        for (auto it = dataObj.begin(); it != dataObj.end(); ++it) {
            n.data[it.key()] = it.value().toVariant();
        }
        n.dismissible = obj["dismissible"].toBool();
        n.persistent = obj["persistent"].toBool();
        n.timeoutMs = obj["timeoutMs"].toInt();
        n.actionText = obj["actionText"].toString();

        if (n.persistent) {
            d->model->addNotification(n);
        }
    }

    return true;
}

// ============================================================================
// NotificationModel Implementation
// ============================================================================

class NotificationModel::Private
{
public:
    QVector<Notification> notifications;
    std::unordered_set<QString> readIds;
    std::mutex mutex;
};

NotificationModel::~NotificationModel() = default;

NotificationModel::NotificationModel(QObject* parent)
    : QObject(parent), d(std::make_unique<Private>())
{
}


int NotificationModel::count() const
{
    std::lock_guard<std::mutex> lock(d->mutex);
    return d->notifications.size();
}

int NotificationModel::unreadCount() const
{
    std::lock_guard<std::mutex> lock(d->mutex);
    int count = 0;
    for (const auto& n : d->notifications) {
        if (d->readIds.find(n.id) == d->readIds.end()) {
            ++count;
        }
    }
    return count;
}

Notification NotificationModel::notificationAt(int index) const
{
    std::lock_guard<std::mutex> lock(d->mutex);
    if (index >= 0 && index < d->notifications.size()) {
        return d->notifications[index];
    }
    return Notification{};
}

QVector<Notification> NotificationModel::notifications() const
{
    std::lock_guard<std::mutex> lock(d->mutex);
    return d->notifications;
}

void NotificationModel::addNotification(const Notification& notification)
{
    std::lock_guard<std::mutex> lock(d->mutex);
    d->notifications.prepend(notification); // Newest first
    emit countChanged();
    if (d->readIds.find(notification.id) == d->readIds.end()) {
        emit unreadCountChanged();
    }
    emit notificationsChanged();
}

void NotificationModel::removeNotification(const QString& id)
{
    std::lock_guard<std::mutex> lock(d->mutex);
    auto it = std::find_if(d->notifications.begin(), d->notifications.end(),
        [&id](const Notification& n) { return n.id == id; });
    if (it != d->notifications.end()) {
        d->notifications.erase(it);
        d->readIds.erase(id);
        emit countChanged();
        emit unreadCountChanged();
        emit notificationsChanged();
    }
}

void NotificationModel::updateNotification(const Notification& notification)
{
    std::lock_guard<std::mutex> lock(d->mutex);
    auto it = std::find_if(d->notifications.begin(), d->notifications.end(),
        [&notification](const Notification& n) { return n.id == notification.id; });
    if (it != d->notifications.end()) {
        *it = notification;
        emit notificationsChanged();
    }
}

void NotificationModel::clear()
{
    std::lock_guard<std::mutex> lock(d->mutex);
    d->notifications.clear();
    d->readIds.clear();
    emit countChanged();
    emit unreadCountChanged();
    emit notificationsChanged();
}

void NotificationModel::markAsRead(const QString& id)
{
    std::lock_guard<std::mutex> lock(d->mutex);
    d->readIds.insert(id);
    emit unreadCountChanged();
}

} // namespace arete::notifications