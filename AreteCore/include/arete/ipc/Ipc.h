#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QStringList>
#include <QVariantMap>
#include <QJsonObject>
#include <functional>
#include <memory>

class QLocalServer;
class QLocalSocket;

namespace arete::ipc {

// ============================================================================
// URI scheme parsing
// ============================================================================
// arete://logos/John/3/16
// arete://codex/note/DailyJournal
// arete://chronos/session/start
// arete://phonio/playlist/Focus

struct UriAction
{
    QString app;         // e.g. "logos", "codex"
    QString command;     // e.g. "John/3/16", "note/DailyJournal"
    QStringList segments; // split command by '/'
    QVariantMap params;  // query parameters (after '?')

    [[nodiscard]] bool isValid() const { return !app.isEmpty(); }
};

[[nodiscard]] UriAction parseUri(const QUrl& url);
[[nodiscard]] UriAction parseUri(const QString& url);
[[nodiscard]] QUrl makeUri(const QString& app, const QString& command,
                           const QVariantMap& params = {});

// ============================================================================
// Cross-application discovery
// ============================================================================

// Returns true if the given application is currently running (via its IPC name)
[[nodiscard]] bool isAppRunning(const QString& appName);

// List of known Arete applications
[[nodiscard]] QStringList knownApps();

// ============================================================================
// Local IPC server (single instance + incoming URIs)
// ============================================================================

class IpcServer : public QObject
{
    Q_OBJECT

public:
    using MessageHandler = std::function<void(const QJsonObject&)>;

    // serverName should be the app id, e.g. "arete-logos"
    explicit IpcServer(const QString& serverName, QObject* parent = nullptr);
    ~IpcServer() override;

    // Try to start the server. Returns false if another instance owns the name.
    [[nodiscard]] bool start();

    // Notify the running instance (returns true if delivered)
    [[nodiscard]] bool notify(const QString& message, const QVariantMap& params = {});

    void setMessageHandler(MessageHandler handler);

signals:
    void messageReceived(const QJsonObject& message);
    void uriReceived(const QString& uri);

private:
    void onNewConnection();
    void onReadyRead();

    class Private;
    std::unique_ptr<Private> d;
};

// ============================================================================
// Convenience: process a URI on startup
// ============================================================================

// If another instance is already running, forwards the URI and returns true.
// Otherwise returns false so the caller can open the URI itself.
[[nodiscard]] bool forwardUriToRunningInstance(const QString& serverName, const QUrl& uri);

} // namespace arete::ipc