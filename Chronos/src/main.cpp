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
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("Chronos"));

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
    window.show();

    return app.exec();
}
