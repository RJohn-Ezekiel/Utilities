#pragma once

#include <QStringList>
#include <QCoreApplication>

namespace chronos {

enum class CliAction {
    LaunchGui,
    PrintHelp,
    PrintVersion,
    PrintStats
};

struct CliResult {
    CliAction action = CliAction::LaunchGui;
};

CliResult parseCommandLine(const QStringList& args);

void printHelp();
void printVersion();
void printStats(const QString& dataDir);

} // namespace chronos
