#include "CommandLineParser.h"

#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace chronos {

CliResult parseCommandLine(const QStringList& args)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "Chronos \u2014 Minimalist focus companion."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption statsOption(
        QStringLiteral("stats"),
        QStringLiteral("Display today's statistics and exit."));
    parser.addOption(statsOption);

    QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Save a PNG of the main window to <path> and exit."),
        QStringLiteral("path"));
    parser.addOption(screenshotOption);

    parser.process(args);

    CliResult result;

    if (parser.isSet(QStringLiteral("help"))) {
        result.action = CliAction::PrintHelp;
        parser.showHelp();
    } else if (parser.isSet(QStringLiteral("version"))) {
        result.action = CliAction::PrintVersion;
        parser.showVersion();
    } else if (parser.isSet(statsOption)) {
        result.action = CliAction::PrintStats;
    }

    return result;
}

void printHelp()
{
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "Chronos \u2014 Minimalist focus companion."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption(
        QStringLiteral("stats"),
        QStringLiteral("Display today's statistics and exit.")));
    parser.showHelp();
}

void printVersion()
{
    qInfo().noquote() << QStringLiteral("Chronos")
                      << QCoreApplication::applicationVersion();
}

void printStats(const QString& dataDir)
{
    QFile file(dataDir + QStringLiteral("/statistics.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        qInfo().noquote() << QStringLiteral("No statistics available yet.");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        qInfo().noquote() << QStringLiteral("No statistics available.");
        return;
    }

    QJsonObject obj = doc.object();
    QJsonObject today = obj[QStringLiteral("today")].toObject();

    qint64 focusSecs = today[QStringLiteral("focusSeconds")].toInteger(0);
    qint64 breakSecs = today[QStringLiteral("breakSeconds")].toInteger(0);
    int sessions = today[QStringLiteral("sessionsCompleted")].toInt(0);
    int tasks = today[QStringLiteral("tasksCompleted")].toInt(0);
    int streak = obj[QStringLiteral("currentStreakDays")].toInt(0);

    auto fmt = [](qint64 secs) -> QString {
        int h = static_cast<int>(secs / 3600);
        int m = static_cast<int>((secs % 3600) / 60);
        if (h > 0) return QStringLiteral("%1h %2m").arg(h).arg(m);
        return QStringLiteral("%1m").arg(m);
    };

    qInfo().noquote() << QStringLiteral("=== Today ===");
    qInfo().noquote() << QStringLiteral("Focus Time:  ") << fmt(focusSecs);
    qInfo().noquote() << QStringLiteral("Break Time:  ") << fmt(breakSecs);
    qInfo().noquote() << QStringLiteral("Sessions:    ") << sessions;
    qInfo().noquote() << QStringLiteral("Tasks Done:  ") << tasks;
    qInfo().noquote() << QStringLiteral("Streak:      %1 days").arg(streak);
}

} // namespace chronos
