#include "arete/update/UpdateService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QSaveFile>
#include <QStandardPaths>

#include <cstdio>
#include <sys/stat.h>

#ifndef ARETE_SOURCE_ROOT
#define ARETE_SOURCE_ROOT ""
#endif

namespace arete::update {

UpdateService::UpdateService(QString appName, QString buildBinaryName, QStringList dataFiles)
    : m_appName(std::move(appName))
    , m_buildBinaryName(buildBinaryName.isEmpty() ? m_appName : std::move(buildBinaryName))
    , m_dataFiles(std::move(dataFiles))
{
}

QString UpdateService::workspaceRoot()
{
    const QByteArray env = qgetenv("ARETE_ROOT");
    if (!env.isEmpty())
        return QString::fromLocal8Bit(env);
    const QString compile = QStringLiteral(ARETE_SOURCE_ROOT);
    if (!compile.isEmpty())
        return compile;
    // Fallback: <workspace>/build/bin/<app>  ->  up two levels
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../..");
}

QString UpdateService::installDir()
{
    const QByteArray env = qgetenv("ARETE_INSTALL_DIR");
    if (!env.isEmpty())
        return QString::fromLocal8Bit(env);
    return QDir::homePath() + "/.local/bin";
}

QString UpdateService::buildBinaryPath() const
{
    return QDir(workspaceRoot()).filePath("build/bin/" + m_buildBinaryName);
}

QString UpdateService::installedBinaryPath() const
{
    return QDir(installDir()).filePath(m_appName + ".bin");
}

UpdateInfo UpdateService::check() const
{
    UpdateInfo info;
    info.appName = m_appName;
    info.dataFiles = m_dataFiles;

    const QString buildPath = buildBinaryPath();
    const QString installPath = installedBinaryPath();

    const QFileInfo buildInfo(buildPath);
    info.buildExists = buildInfo.exists();
    if (info.buildExists) {
        info.buildPath = buildInfo.absoluteFilePath();
        info.buildTime = buildInfo.lastModified();
    }

    const QFileInfo installInfo(installPath);
    info.installed = installInfo.exists();
    if (info.installed) {
        info.installPath = installInfo.absoluteFilePath();
        info.installTime = installInfo.lastModified();
    }

    if (!info.buildExists) {
        info.message = QStringLiteral("No build found in the build tree (%1).")
                           .arg(QDir(workspaceRoot()).filePath("build/bin"));
        return info;
    }

    // A data file that is fresher than its installed counterpart also counts.
    info.updateAvailable = !info.installed || info.buildTime > info.installTime;
    if (!info.updateAvailable) {
        for (const QString& data : m_dataFiles) {
            const QFileInfo b(QDir(QFileInfo(buildPath).absolutePath()).filePath(data));
            const QFileInfo i(QDir(installDir()).filePath(data));
            if (b.exists() && (!i.exists() || b.lastModified() > i.lastModified())) {
                info.updateAvailable = true;
                break;
            }
        }
    }

    if (info.updateAvailable) {
        info.message = QStringLiteral("Newer build available: %1 (build %2, installed %3).")
                           .arg(m_appName,
                                info.buildTime.toString(QStringLiteral("yyyy-MM-dd HH:mm")),
                                info.installed ? info.installTime.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
                                               : QStringLiteral("— not installed"));
    } else {
        info.message = QStringLiteral("You are up to date (%1).")
                           .arg(info.buildTime.toString(QStringLiteral("yyyy-MM-dd HH:mm")));
    }
    return info;
}

bool UpdateService::apply(const UpdateInfo& info, QString* error) const
{
    auto fail = [&](const QString& msg) {
        if (error)
            *error = msg;
        return false;
    };

    const QDir install(installDir());
    if (!install.exists() && !QDir().mkpath(install.absolutePath()))
        return fail(QStringLiteral("Could not create install directory %1").arg(install.absolutePath()));

    // Copy the binary atomically (rename over a running file works on Linux;
    // the running process keeps the old inode).
    {
        QSaveFile out(QDir(install.absolutePath()).filePath(m_appName + ".bin"));
        if (!out.open(QIODevice::WriteOnly)) {
            out.cancelWriting();
            return fail(QStringLiteral("Could not write %1").arg(out.fileName()));
        }
        QFile src(info.buildPath);
        if (!src.open(QIODevice::ReadOnly)) {
            out.cancelWriting();
            return fail(QStringLiteral("Could not read %1").arg(info.buildPath));
        }
        out.write(src.readAll());
        src.close();
        if (!out.commit()) {
            out.cancelWriting();
            return fail(QStringLiteral("Could not commit %1").arg(out.fileName()));
        }
        ::chmod(QDir(install.absolutePath()).filePath(m_appName + ".bin").toLocal8Bit().constData(), 0755);
    }

    // Copy the auxiliary data files.
    for (const QString& data : m_dataFiles) {
        const QFileInfo b(QDir(QFileInfo(info.buildPath).absolutePath()).filePath(data));
        if (!b.exists())
            continue;
        const QString dest = QDir(install.absolutePath()).filePath(data);
        QFile::remove(dest);
        if (!QFile::copy(b.absoluteFilePath(), dest))
            return fail(QStringLiteral("Could not copy %1").arg(data));
    }

    return true;
}

void UpdateService::promptAndApply(QWidget* parent, QString appName,
                                   QString buildBinaryName, QStringList dataFiles)
{
    UpdateService service(std::move(appName), std::move(buildBinaryName), std::move(dataFiles));
    const UpdateInfo info = service.check();

    if (!info.buildExists) {
        QMessageBox::warning(parent, QStringLiteral("Update %1").arg(info.appName), info.message);
        return;
    }

    if (!info.updateAvailable) {
        QMessageBox::information(parent, QStringLiteral("Update %1").arg(info.appName), info.message);
        return;
    }

    const auto choice = QMessageBox::question(
        parent,
        QStringLiteral("Update %1").arg(info.appName),
        QStringLiteral("%1\n\nInstall now and restart to apply?").arg(info.message));
    if (choice != QMessageBox::Yes)
        return;

    QString error;
    if (service.apply(info, &error)) {
        QMessageBox::information(
            parent,
            QStringLiteral("Update %1").arg(info.appName),
            QStringLiteral("%1 updated to the build of %2.\n\nPlease restart the application to use it.")
                .arg(info.appName, info.buildTime.toString(QStringLiteral("yyyy-MM-dd HH:mm"))));
    } else {
        QMessageBox::critical(parent, QStringLiteral("Update %1").arg(info.appName), error);
    }
}

UpdateButton::UpdateButton(QString appName, QString buildBinaryName, QStringList dataFiles, QWidget* parent)
    : QPushButton(QStringLiteral("Update"), parent)
    , m_appName(std::move(appName))
    , m_buildBinaryName(buildBinaryName.isEmpty() ? m_appName : std::move(buildBinaryName))
    , m_dataFiles(std::move(dataFiles))
{
    setToolTip(QStringLiteral("Check the build tree for a newer version of %1 and install it").arg(m_appName));
    setFixedHeight(26);
    connect(this, &QPushButton::clicked, this, &UpdateButton::checkAndUpdate);
}

void UpdateButton::checkAndUpdate()
{
    UpdateService::promptAndApply(this, m_appName, m_buildBinaryName, m_dataFiles);
}

} // namespace arete::update
