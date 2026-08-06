#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QVersionNumber>

namespace arete::services {

// Checks GitHub for a newer Arete release and, when online, downloads and
// installs the published binary onto the installed location, then asks the
// user to restart. All state is reported through stateChanged()/state().
class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,          // nothing done yet
        Checking,      // querying the GitHub API
        Offline,       // no internet / API unreachable
        UpToDate,      // installed binary is current
        UpdateAvailable, // a newer release exists and can be installed
        Downloading,   // fetching the release asset
        ReadyToRestart, // binary replaced; restart Arete to finish
        Failed,        // something went wrong; see error()
    };
    Q_ENUM(State)

    explicit UpdateChecker(const QString& githubRepo, const QString& releaseAsset,
                           const QString& currentVersion, QObject* parent = nullptr);

    void check();
    void install();

    State state() const { return m_state; }
    QString latestVersion() const { return m_latestVersion; }
    QString error() const { return m_error; }

signals:
    void stateChanged();

private:
    void setState(State state, const QString& error = QString());
    void finishCheck(QNetworkReply* reply);
    void finishAsset(QNetworkReply* reply);

    QString m_repo;
    QString m_assetName;
    QString m_currentVersion;
    State m_state = State::Idle;
    QString m_latestVersion;
    QString m_error;
    QString m_assetUrl;
    QNetworkAccessManager m_net;
    QByteArray m_downloadBuffer;
};

} // namespace arete::services