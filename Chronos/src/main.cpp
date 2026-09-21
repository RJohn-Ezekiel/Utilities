#include <QApplication>
#include <QFont>

#include "storage/StorageManager.h"
#include "services/TimerService.h"
#include "services/TaskService.h"
#include "services/StatisticsService.h"
#include "services/AudioService.h"
#include "services/ReminderScheduler.h"
#include "services/NotificationService.h"
#include "ui/MainWindow.h"
#include "cli/CommandLineParser.h"

#include "arete/ipc/Ipc.h"
#include "arete/logging/Logger.h"

// Route Qt's own debug output into the internal logger instead of the terminal.
static void qtMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
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

__attribute__((constructor))
static void earlySuppress()
{
    qputenv("QT_LOGGING_RULES", "kf.*.warning=false");
    qputenv("QT_QPA_PLATFORMTHEME", "");
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Chronos"));
    app.setApplicationVersion(QStringLiteral("1.1.0"));
    app.setOrganizationName(QStringLiteral("Arete"));

    // Internal logging: never print to the terminal during normal operation.
    auto& logger = arete::logging::Logger::instance();
    logger.setMinLevel(arete::logging::Level::Info);
    qInstallMessageHandler(qtMessageHandler);

    // ── CLI (non-GUI actions) ──
    auto cliResult = chronos::parseCommandLine(app.arguments());
    switch (cliResult.action) {
    case chronos::CliAction::PrintStats: {
        auto storage = chronos::StorageManager();
        chronos::printStats(storage.dataDirectory());
        return 0;
    }
    case chronos::CliAction::PrintHelp:
        chronos::printHelp();
        return 0;
    case chronos::CliAction::PrintVersion:
        chronos::printVersion();
        return 0;
    case chronos::CliAction::LaunchGui:
        break; // continue below
    }

    // ── Single instance + deep-linking (arete://chronos/session/start) ──
    static arete::ipc::IpcServer ipc(QStringLiteral("arete-chronos"));
    if (!ipc.start()) {
        const auto args = app.arguments();
        for (const QString& arg : args) {
            if (arg.startsWith(QStringLiteral("arete://"))) {
                const bool forwarded = arete::ipc::forwardUriToRunningInstance(QStringLiteral("arete-chronos"), QUrl(arg));
                Q_UNUSED(forwarded)
                return 0;
            }
        }
    }

    // ── Font ──
    QFont monoFont(QStringLiteral("JetBrains Mono"));
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setPointSize(13);
    app.setFont(monoFont);

    // ── Services ──
    auto* storage    = new chronos::StorageManager(&app);
    auto* timerSvc   = new chronos::TimerService(storage, &app);
    auto* taskSvc    = new chronos::TaskService(storage, &app);
    auto* statsSvc   = new chronos::StatisticsService(storage, taskSvc, &app);
    auto* audioSvc   = new chronos::AudioService(&app);
    auto* reminders  = new chronos::ReminderScheduler(&app);
    auto* notifSvc   = new chronos::NotificationService(audioSvc, &app);

    reminders->configure(timerSvc->currentSettings());
    timerSvc->attemptRestore();

    // ── UI ──
    chronos::MainWindow window(timerSvc, taskSvc, statsSvc, reminders, notifSvc);

    // Handle deep links
    const auto args = app.arguments();
    for (const QString& arg : args) {
        if (arg.startsWith(QStringLiteral("arete://"))) {
            const auto action = arete::ipc::parseUri(arg);
            if (action.app == QStringLiteral("chronos") && action.command == QStringLiteral("session/start")) {
                timerSvc->startFocus();
            }
        }
    }

    window.show();

    // Dev helper: render the window to a PNG and exit (used for visual checks).
    const auto shotIdx = args.indexOf(QStringLiteral("--screenshot"));
    if (shotIdx != -1 && shotIdx + 1 < args.size()) {
        window.grab().save(args.at(shotIdx + 1));
        return 0;
    }

    return app.exec();
}
