#include "cli/CLIHandler.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include "arete/logging/Logger.h"
#include "arete/ipc/Ipc.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QLoggingCategory>
#include <QStandardPaths>

#include <string>

namespace {

std::string resolveBiblesDir()
{
    // 1. Baked-in compile-time path (development / build tree)
    QString dir = QStringLiteral(BIBLES_DIR);
    if (QDir(dir).exists())
        return QDir(dir).absolutePath().toStdString();

    // 2. Next to executable (portable / installed alongside bin)
    dir = QCoreApplication::applicationDirPath() + "/Bibles";
    if (QDir(dir).exists())
        return QDir(dir).absolutePath().toStdString();

    // 3. One level up from executable (build/bible → build/../Bibles)
    dir = QCoreApplication::applicationDirPath() + "/../Bibles";
    if (QDir(dir).exists())
        return QDir(dir).absolutePath().toStdString();

    // 4. XDG data location (system-wide install)
    dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Bibles";
    if (QDir(dir).exists())
        return QDir(dir).absolutePath().toStdString();

    // 5. Current working directory (last resort)
    dir = QDir::currentPath() + "/Bibles";
    return QDir(dir).absolutePath().toStdString();
}

// Route Qt's own debug output into the internal logger instead of the terminal.
void qtMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    using namespace arete::logging;
    auto& logger = Logger::instance();
    switch (type) {
        case QtDebugMsg:    logger.debug(msg, QStringLiteral("qt")); break;
        case QtInfoMsg:     logger.info(msg, QStringLiteral("qt")); break;
        case QtWarningMsg:  logger.warning(msg, QStringLiteral("qt")); break;
        case QtCriticalMsg: logger.error(msg, QStringLiteral("qt")); break;
        case QtFatalMsg:    logger.critical(msg, QStringLiteral("qt")); break;
    }
}

} // namespace

__attribute__((constructor))
static void earlySuppress()
{
    qputenv("QT_LOGGING_RULES", "kf.*.warning=false");
    qputenv("QT_QPA_PLATFORMTHEME", "");
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Logos");
    app.setApplicationVersion("1.1.0");
    app.setOrganizationName("Arete");

    QFont defaultFont("JetBrains Mono", 10);
    defaultFont.setStyleHint(QFont::Monospace);
    app.setFont(defaultFont);

    // Internal logging: never print to the terminal during normal operation.
    auto& logger = arete::logging::Logger::instance();
    logger.setMinLevel(arete::logging::Level::Info);
    logger.setLogToFile(true);
    qInstallMessageHandler(qtMessageHandler);

    std::string biblesDir = resolveBiblesDir();

    auto config = CLIHandler::parse(argc, argv);
    config.biblesPath = biblesDir;

    if (config.mode != CLIHandler::Mode::GUI &&
        config.mode != CLIHandler::Mode::Prayer &&
        config.mode != CLIHandler::Mode::Hymn) {
        return CLIHandler::execute(config);
    }

    // Single-instance + deep-linking (arete://logos/John/3/16)
    static arete::ipc::IpcServer ipc(QStringLiteral("arete-logos"));
    if (!ipc.start()) {
        // Another instance is running: forward the URI and exit.
        const auto args = app.arguments();
        for (const QString& arg : args) {
            if (arg.startsWith(QStringLiteral("arete://"))) {
                const bool forwarded =
                    arete::ipc::forwardUriToRunningInstance(QStringLiteral("arete-logos"), QUrl(arg));
                Q_UNUSED(forwarded);
                return 0;
            }
        }
    }

    Theme::apply();

    MainWindow window;
    window.loadBibles(biblesDir);

    if (config.mode == CLIHandler::Mode::Prayer) {
        window.openPrayerMode();
        if (!config.query.empty())
            window.selectPrayerByText(QString::fromStdString(config.query));
    } else if (config.mode == CLIHandler::Mode::Hymn) {
        window.openHymnMode();
        if (!config.query.empty())
            window.selectHymnByText(QString::fromStdString(config.query));
    }

    // Handle deep links from the CLI
    QStringList pendingUris;
    const auto args = app.arguments();
    for (const QString& arg : args) {
        if (arg.startsWith(QStringLiteral("arete://"))) {
            pendingUris.append(arg);
        }
    }
    if (!pendingUris.isEmpty()) {
        window.openUri(pendingUris.first());
    }

    QObject::connect(&ipc, &arete::ipc::IpcServer::uriReceived, &window, [&window](const QString& uri) {
        window.openUri(uri);
    });

    window.show();

    // Dev helper: render the window to a PNG and exit (used for visual checks).
    const auto shotIdx = args.indexOf(QStringLiteral("--screenshot"));
    if (shotIdx != -1 && shotIdx + 1 < args.size()) {
        window.grab().save(args.at(shotIdx + 1));
        return 0;
    }

    return app.exec();
}