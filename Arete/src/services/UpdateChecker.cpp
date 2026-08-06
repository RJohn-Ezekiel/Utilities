#include "services/UpdateChecker.h"
#include "arete/update/UpdateService.h"
#include "arete/logging/Logger.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

using arete::logging::Logger;

namespace arete::services {

UpdateChecker::UpdateChecker(const QString& githubRepo, const QString& releaseAsset,
                             const QString& currentVersion, QObject* parent)
    : QObject(parent), m_repo(githubRepo), m_assetName(releaseAsset),
      m_currentVersion(currentVersion)
{
}

void UpdateChecker::check()
{
    if (m_state == State::Checking || m_state == State::Downloading) return;

    setState(State::Checking);
    m_latestVersion.clear();
    m_assetUrl.clear();
    m_error.clear();

    QNetworkRequest request(
        QUrl(QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(m_repo)));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Arete-updater/") + m_currentVersion);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setTransferTimeout(20000);

    QNetworkReply* reply = m_net.get(request);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply] { finishCheck(reply); });
}

void UpdateChecker::finishCheck(QNetworkReply* reply)
{
    const QNetworkReply::NetworkError error = reply->error();
    const int status =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    reply->deleteLater();

    if (error != QNetworkReply::NoError) {
        // HTTP errors mean the network is fine but the answer is an error;
        // anything else is a genuine connectivity problem.
        if (status > 0) {
            if (status == 404) {
                setState(State::Failed, QStringLiteral("No release published yet"));
            } else {
                setState(State::Failed, QStringLiteral("GitHub answered HTTP %1").arg(status));
            }
        } else {
            setState(State::Offline, reply->errorString());
        }
        return;
    }
    if (status != 200) {
        // The API answered; a 404 simply means no release has been published.
        setState(State::Failed, status == 404
                                   ? QStringLiteral("No release published yet")
                                   : QStringLiteral("GitHub answered HTTP %1").arg(status));
        return;
    }

    const QJsonObject root = QJsonDocument::fromJson(body).object();
    QString tag = root.value(QStringLiteral("tag_name")).toString();
    tag = tag.startsWith(QLatin1Char('v')) ? tag.mid(1) : tag;
    m_latestVersion = tag;

    const QJsonArray assets = root.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue& value : assets) {
        const QJsonObject asset = value.toObject();
        if (asset.value(QStringLiteral("name")).toString() == m_assetName) {
            m_assetUrl = asset.value(QStringLiteral("browser_download_url")).toString();
            break;
        }
    }

    if (m_latestVersion.isEmpty()) {
        setState(State::Failed, QStringLiteral("Release has no version tag"));
        return;
    }
    if (m_assetUrl.isEmpty()) {
        setState(State::Failed,
                 QStringLiteral("Release %1 has no %2 asset").arg(m_latestVersion, m_assetName));
        return;
    }

    const QVersionNumber latest = QVersionNumber::fromString(m_latestVersion);
    const QVersionNumber current = QVersionNumber::fromString(m_currentVersion);
    if (latest.isNull() || latest <= current) {
        setState(State::UpToDate);
        return;
    }
    setState(State::UpdateAvailable);
}

void UpdateChecker::install()
{
    if (m_assetUrl.isEmpty() || m_state != State::UpdateAvailable) return;

    setState(State::Downloading);
    m_downloadBuffer.clear();

    QNetworkRequest request{QUrl(m_assetUrl)};
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Arete-updater/") + m_currentVersion);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(60000);

    QNetworkReply* reply = m_net.get(request);
    connect(reply, &QNetworkReply::readyRead, this,
            [this, reply] { m_downloadBuffer += reply->readAll(); });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply] { finishAsset(reply); });
}

void UpdateChecker::finishAsset(QNetworkReply* reply)
{
    const QNetworkReply::NetworkError error = reply->error();
    m_downloadBuffer += reply->readAll();
    reply->deleteLater();

    if (error != QNetworkReply::NoError) {
        setState(State::Failed, reply->errorString());
        return;
    }
    if (m_downloadBuffer.isEmpty()) {
        setState(State::Failed, QStringLiteral("Downloaded file is empty"));
        return;
    }

    // Replace the installed binary atomically; the running process keeps its
    // own inode, so this is safe while Arete is open.
    const QString installPath =
        arete::update::UpdateService::installDir() + QStringLiteral("/arete.bin");
    const QString tmpPath = QDir::tempPath() + QStringLiteral("/arete.bin.new");
    {
        QFile tmp(tmpPath);
        if (!tmp.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setState(State::Failed, QStringLiteral("Cannot write temporary file"));
            return;
        }
        tmp.write(m_downloadBuffer);
        tmp.flush();
        tmp.close();
    }
    QFile::setPermissions(tmpPath,
                          QFile::permissions(tmpPath) | QFile::ExeOwner | QFile::ExeGroup
                              | QFile::ExeOther);
    if (!QFile::rename(tmpPath, installPath)) {
        QFile::remove(tmpPath);
        setState(State::Failed, QStringLiteral("Cannot replace %1").arg(installPath));
        return;
    }

    Logger::instance().info(QStringLiteral("Self-updated to %1").arg(m_latestVersion),
                            QStringLiteral("update"));
    setState(State::ReadyToRestart);
}

void UpdateChecker::setState(State state, const QString& error)
{
    if (m_state != state) {
        m_state = state;
        if (!error.isEmpty()) m_error = error;
        emit stateChanged();
    }
}

} // namespace arete::services