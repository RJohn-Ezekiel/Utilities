#pragma once

#include <QObject>
#include <QString>

namespace arete::services {

// Local-only self-update for Arete. Scans Arete's own source tree under
// ARETE_SOURCE_ROOT; when any source file is newer than the binary installed
// on ~/.local/bin, recompiles Arete from those local sources and atomically
// swaps the fresh binary onto the installed location, then asks the user to
// restart. All state is reported through stateChanged()/state(). No GitHub,
// no network.
class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Idle,           // nothing done yet
        Checking,       // scanning the local source tree for changes
        Offline,        // kept for signature compat; never used locally
        UpToDate,       // installed binary is newer than every local source
        UpdateAvailable, // a local source changed; recompile + swap can be applied
        Downloading,    // kept for signature compat; never used locally
        Compiling,      // rebuilding Arete from the local sources
        ReadyToRestart, // binary replaced; restart Arete to finish
        Failed,         // something went wrong; see error()
    };
    Q_ENUM(State)

    explicit UpdateChecker(const QString& githubRepo, const QString& releaseAsset,
                           const QString& currentVersion, QObject* parent = nullptr);

    // Point the checker at Arete's local source tree (the monorepo root that
    // contains Arete/ and AreteCore/). Optional: call before check(). When
    // unset, the updater is a no-op legacy GitHub stub for signature compat.
    void setSourceRoot(const QString& root);

    void check();
    void install();

    State state() const { return m_state; }
    QString latestVersion() const { return m_latestVersion; }
    QString error() const { return m_error; }

signals:
    void stateChanged();

private:
    void setState(State state, const QString& error = QString());

    QString m_repo;       // kept for signature compat; unused locally
    QString m_assetName;  // kept for signature compat; unused locally
    QString m_currentVersion;
    QString m_sourceRoot;
    State m_state = State::Idle;
    QString m_latestVersion;
    QString m_error;
};

} // namespace arete::services
