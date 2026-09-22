#include "services/UpdateChecker.h"

#include "arete/update/UpdateService.h"
#include "arete/logging/Logger.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QThread>
#include <QtGlobal>

using arete::logging::Logger;
using arete::update::UpdateService;

namespace arete::services {

namespace {

QString sourceRoot()
{
    return QString::fromLocal8Bit(qgetenv("ARETE_SOURCE_ROOT"));
}

QString installDir()
{
    return UpdateService::installDir();
}

// Newest *source* mtime anywhere under the given source root; 0 when the
// directory doesn't exist or has no matching files.
qint64 newestSourceMtime(const QString& root)
{
    qint64 newest = 0;
    QDirIterator it(root,
                    {QStringLiteral("*.cpp"), QStringLiteral("*.h"), QStringLiteral("*.hpp"),
                     QStringLiteral("*.ui"), QStringLiteral("*.qrc")},
                    QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        newest = qMax(newest, it.fileInfo().lastModified().toMSecsSinceEpoch());
    }
    return newest;
}

} // namespace

UpdateChecker::UpdateChecker(const QString& repo, const QString& asset,
                             const QString& currentVersion, QObject* parent)
    : QObject(parent), m_repo(repo), m_assetName(asset), m_currentVersion(currentVersion)
{
}

void UpdateChecker::setState(State state, const QString& error)
{
    if (m_state == state) return;
    m_state = state;
    m_error = error;
    emit stateChanged();
}

void UpdateChecker::check()
{
    if (m_state == State::Checking || m_state == State::Compiling)
        return;

    setState(State::Checking);

    const QString root = sourceRoot();
    if (root.isEmpty()) {
        setState(State::Offline,
                 QStringLiteral("ARETE_SOURCE_ROOT is not set; cannot scan local sources"));
        return;
    }
    m_sourceRoot = root;

    const QString installed = installDir() + QStringLiteral("/arete.bin");
    const QFileInfo installedInfo(installed);
    if (!installedInfo.exists()) {
        setState(State::UpdateAvailable,
                 QStringLiteral("Arete is not installed on %1 yet").arg(installed));
        return;
    }

    const qint64 src = newestSourceMtime(root);
    const qint64 bin = installedInfo.lastModified().toMSecsSinceEpoch();
    if (src <= bin) {
        m_latestVersion = m_currentVersion;
        setState(State::UpToDate);
        return;
    }

    // Some local source changed since the installed binary was built.
    m_latestVersion = m_currentVersion + QStringLiteral("-local");
    setState(State::UpdateAvailable);
}

void UpdateChecker::install()
{
    if (m_state != State::UpdateAvailable || m_sourceRoot.isEmpty()) {
        setState(State::Failed,
                 m_sourceRoot.isEmpty()
                     ? QStringLiteral("No local source root configured")
                     : QStringLiteral("Nothing to install"));
        return;
    }

    setState(State::Compiling);

    const QString buildDir = m_sourceRoot + QStringLiteral("/build");
    const QString installPath = installDir() + QStringLiteral("/arete.bin");
    const QString bakPath = installPath + QStringLiteral(".bak");
    const QString tmpPath = installPath + QStringLiteral(".new");

    // Rebuild Arete from the local sources (cmake --build), then atomically
    // swap the fresh binary over the installed one)Skip.
    QProcess* builder = new QProcess(this);
    connect(builder, &QProcess::finished, this, [this, builder, buildDir,
                                                 installPath, bakPath, tmpPath](
                                                int exitCode) {
        const QByteArray log = builder->readAllStandardOutput()
                               + builder->readAllStandardError();
        builder->deleteLater();

        if (exitCode != 0) {
            setState(State::Failed,
                     QStringLiteral("Build failed (exit %1):\n%2")
                         .arg(exitCode)
                         .arg(QString::fromLocal8Bit(log).trimmed()));
            return;
        }

        const QString freshPath = buildDir + QStringLiteral("/bin/arete");
        if (!QFile::exists(freshPath)) {
            setState(State::Failed,
                     QStringLiteral("No binary produced at %1").arg(freshPath));
            return;
        }

        // Atomic swap: keep a .bak of the current install in case the fresh
        // build is bad, stage the fresh binary, then rename it over the
        // installed location.
        QFile::remove(bakPath);
        QFile::setPermissions(installPath,
                              QFile::permissions(installPath)
                                  | QFile::WriteOwner | QFile::ReadOwner);
        QFile::copy(installPath, bakPath);
        QFile::remove(tmpPath);
        QFile::rename(freshPath, tmpPath);
        QFile::setPermissions(tmpPath, QFile::permissions(tmpPath)
                                           | QFile::ExeOwner | QFile::ExeGroup
                                           | QFile::ExeOther);
        if (!QFile::remove(installPath) || !QFile::rename(tmpPath, installPath)) {
            setState(State::Failed, QStringLiteral("Cannot replace %1").arg(installPath));
            return;
        }

        Logger::instance().info(QStringLiteral("Reinstalled from local sources"),
                                QStringLiteral("update"));
        setState(State::ReadyToRestart);
    });
    builder->setWorkingDirectory(buildDir);
    builder->setProgram(QStringLiteral("cmake"));
    builder->setArguments({QStringLiteral("--build"), buildDir,
                           QStringLiteral("--target"), QStringLiteral("arete"),
                           QStringLiteral("-j"), QString::number(QThread::idealThreadCount())});
    builder->start();
}

} // namespace arete::services
