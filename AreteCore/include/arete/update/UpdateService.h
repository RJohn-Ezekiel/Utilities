#pragma once

#include <QDateTime>
#include <QPushButton>
#include <QString>
#include <QStringList>

namespace arete::update {

// Result of a single check: is there a fresher build in the build tree than
// the installed copy in ~/.local/bin, and where do the files live.
struct UpdateInfo {
    bool buildExists = false;     // build tree contains the binary
    bool installed = false;       // an installed copy exists
    bool updateAvailable = false; // build is newer than the installed copy
    QString appName;              // e.g. "logos"
    QString buildPath;            // /home/magesh/Utilities/build/bin/logos
    QString installPath;          // ~/.local/bin/logos.bin
    QStringList dataFiles;        // extra files next to the binary (prayer.json, hymns.json)
    QDateTime buildTime;
    QDateTime installTime;
    QString message;              // human-readable status for dialogs
};

// Locates the shared build tree (ARETE_ROOT env or the ARETE_SOURCE_ROOT
// compile definition) and the per-user install dir (~/.local/bin), compares
// the freshly built binary against the installed one and applies the update
// by atomically replacing the installed files.
class UpdateService
{
public:
    // appName:            "logos" -> installs to ~/.local/bin/logos.bin
    // buildBinaryName:    name of the binary inside build/bin (defaults to
    //                     appName; Visio is "visio_gui")
    // dataFiles:          file names to also copy (relative to build/bin)
    explicit UpdateService(QString appName, QString buildBinaryName = {},
                           QStringList dataFiles = {});

    [[nodiscard]] UpdateInfo check() const;
    // Returns true on success; error contains a description on failure.
    [[nodiscard]] bool apply(const UpdateInfo& info, QString* error = nullptr) const;

    // Runs the full check -> prompt -> install flow as a modal dialog.
    // Use this when the app has no toolbar button (menu/sidebar entries).
    static void promptAndApply(QWidget* parent, QString appName,
                               QString buildBinaryName = {}, QStringList dataFiles = {});

    static QString workspaceRoot();
    static QString installDir();

private:
    [[nodiscard]] QString buildBinaryPath() const;
    [[nodiscard]] QString installedBinaryPath() const;

    QString m_appName;
    QString m_buildBinaryName;
    QStringList m_dataFiles;
};

// A ready-made toolbar button: on click, runs the check, asks the user whether
// to install when a newer build exists and reports the outcome.
class UpdateButton : public QPushButton
{
    Q_OBJECT

public:
    explicit UpdateButton(QString appName, QString buildBinaryName = {},
                          QStringList dataFiles = {}, QWidget* parent = nullptr);

private slots:
    void checkAndUpdate();

private:
    QString m_appName;
    QString m_buildBinaryName;
    QStringList m_dataFiles;
};

} // namespace arete::update
