#include "arete/ipc/Ipc.h"
#include "arete/json/JsonUtils.h"
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDataStream>
#include <QUrlQuery>
#include <QTimer>

namespace arete::ipc {

UriAction parseUri(const QUrl& url)
{
    UriAction action;
    if (url.scheme().toLower() != QStringLiteral("arete")) return action;

    const QString host = url.host().toLower();
    if (host.isEmpty()) return action;

    action.app = host;
    QString path = url.path();
    if (path.startsWith(QLatin1Char('/'))) {
        path = path.mid(1);
    }
    action.command = path;
    action.segments = path.isEmpty() ? QStringList() : path.split(QLatin1Char('/'));

    const QUrlQuery query(url.query());
    const auto items = query.queryItems();
    for (const auto& [key, value] : items) {
        action.params[key] = value;
    }
    return action;
}

UriAction parseUri(const QString& url)
{
    return parseUri(QUrl(url));
}

QUrl makeUri(const QString& app, const QString& command, const QVariantMap& params)
{
    QString path = command;
    if (!path.isEmpty() && !path.startsWith(QLatin1Char('/'))) {
        path.prepend(QLatin1Char('/'));
    }
    QUrl url;
    url.setScheme(QStringLiteral("arete"));
    url.setHost(app);
    url.setPath(path);

    if (!params.isEmpty()) {
        QUrlQuery query;
        for (auto it = params.begin(); it != params.end(); ++it) {
            query.addQueryItem(it.key(), it.value().toString());
        }
        url.setQuery(query);
    }
    return url;
}

bool isAppRunning(const QString& appName)
{
    const QString serverName = appName.startsWith(QStringLiteral("arete-"))
        ? appName : QStringLiteral("arete-%1").arg(appName);
    QLocalSocket socket;
    socket.connectToServer(serverName, QIODevice::ReadWrite);
    if (socket.waitForConnected(100)) {
        socket.disconnectFromServer();
        return true;
    }
    return false;
}

QStringList knownApps()
{
    return {QStringLiteral("logos"), QStringLiteral("phonio"), QStringLiteral("visio"),
            QStringLiteral("codex"), QStringLiteral("chronos")};
}

// ============================================================================
// IpcServer
// ============================================================================

class IpcServer::Private
{
public:
    QString serverName;
    QLocalServer* server = nullptr;
    MessageHandler handler;
    QList<QLocalSocket*> sockets;
    QHash<QLocalSocket*, QByteArray> buffers;
};

IpcServer::IpcServer(const QString& serverName, QObject* parent)
    : QObject(parent), d(std::make_unique<Private>())
{
    d->serverName = serverName;
}

IpcServer::~IpcServer()
{
    if (d->server) {
        d->server->close();
        delete d->server;
    }
    for (QLocalSocket* socket : d->sockets) {
        socket->deleteLater();
    }
}

bool IpcServer::start()
{
    if (!d->serverName.isEmpty()) {
        QLocalServer::removeServer(d->serverName);
    }

    d->server = new QLocalServer(this);
    d->server->setSocketOptions(QLocalServer::UserAccessOption);
    if (!d->server->listen(d->serverName)) {
        delete d->server;
        d->server = nullptr;
        return false;
    }

    connect(d->server, &QLocalServer::newConnection, this, [this]() { onNewConnection(); });
    return true;
}

bool IpcServer::notify(const QString& message, const QVariantMap& params)
{
    QLocalSocket socket;
    socket.connectToServer(d->serverName, QIODevice::WriteOnly);
    if (!socket.waitForConnected(1000)) {
        return false;
    }

    QJsonObject payload;
    payload[QStringLiteral("message")] = message;
    payload[QStringLiteral("params")] = QJsonObject::fromVariantMap(params);

    QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QByteArray sizeData;
    QDataStream sizeStream(&sizeData, QIODevice::WriteOnly);
    sizeStream.setVersion(QDataStream::Qt_6_0);
    sizeStream << static_cast<qint32>(data.size());

    socket.write(sizeData);
    socket.write(data);
    return socket.waitForBytesWritten(1000);
}

void IpcServer::setMessageHandler(MessageHandler handler)
{
    d->handler = std::move(handler);
}

void IpcServer::onNewConnection()
{
    while (d->server && d->server->hasPendingConnections()) {
        QLocalSocket* socket = d->server->nextPendingConnection();
        if (!socket) continue;
        d->sockets.append(socket);
        connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
            d->buffers[socket].append(socket->readAll());

            QByteArray& buffer = d->buffers[socket];
            while (buffer.size() >= 4) {
                QDataStream sizeStream(buffer);
                sizeStream.setVersion(QDataStream::Qt_6_0);
                qint32 size = 0;
                sizeStream >> size;
                if (size <= 0 || size > 10 * 1024 * 1024) {
                    buffer.clear();
                    break;
                }
                if (buffer.size() < static_cast<int>(size) + 4) break;

                const QByteArray payload = buffer.mid(4, size);
                buffer.remove(0, 4 + size);

                QJsonParseError err;
                const QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
                if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;

                const QJsonObject msg = doc.object();
                if (d->handler) {
                    d->handler(msg);
                }
                emit messageReceived(msg);

                // URI forwarding
                const QString uri = msg.value(QStringLiteral("uri")).toString();
                if (!uri.isEmpty()) {
                    emit uriReceived(uri);
                }
            }
        });
        connect(socket, &QLocalSocket::disconnected, this, [this, socket]() {
            d->sockets.removeAll(socket);
            d->buffers.remove(socket);
            socket->deleteLater();
        });
    }
}

void IpcServer::onReadyRead()
{
    // Handled in onNewConnection
}

bool forwardUriToRunningInstance(const QString& serverName, const QUrl& uri)
{
    if (!isAppRunning(serverName)) return false;

    QLocalSocket socket;
    socket.connectToServer(serverName, QIODevice::WriteOnly);
    if (!socket.waitForConnected(1000)) {
        return false;
    }

    QJsonObject payload;
    payload[QStringLiteral("uri")] = uri.toString();

    QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QByteArray sizeData;
    QDataStream sizeStream(&sizeData, QIODevice::WriteOnly);
    sizeStream.setVersion(QDataStream::Qt_6_0);
    sizeStream << static_cast<qint32>(data.size());

    socket.write(sizeData);
    socket.write(data);
    return socket.waitForBytesWritten(1000);
}

} // namespace arete::ipc