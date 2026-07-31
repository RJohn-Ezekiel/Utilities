#include "cli/CLIHandler.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

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
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Logos");

    QFont defaultFont("JetBrains Mono", 10);
    defaultFont.setStyleHint(QFont::Monospace);
    app.setFont(defaultFont);

    std::string biblesDir = resolveBiblesDir();

    auto config = CLIHandler::parse(argc, argv);
    config.biblesPath = biblesDir;

    if (config.mode != CLIHandler::Mode::GUI) {
        return CLIHandler::execute(config);
    }

    Theme::apply();

    MainWindow window;
    window.loadBibles(biblesDir);
    window.show();

    // Dev helper: render the window to a PNG and exit (used for visual checks).
    const auto args = app.arguments();
    const auto shotIdx = args.indexOf(QStringLiteral("--screenshot"));
    if (shotIdx != -1 && shotIdx + 1 < args.size()) {
        window.grab().save(args.at(shotIdx + 1));
        return 0;
    }

    return app.exec();
}
