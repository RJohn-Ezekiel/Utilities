#include "app/Application.h"
#include "arete/logging/Logger.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QDir>
#include <QTimer>
#include <QtGlobal>

namespace {

// Everything Arete and Qt print goes into the in-app log (file + Logs tab)
// instead of the terminal.
void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    auto& logger = arete::logging::Logger::instance();
    arete::logging::Level level = arete::logging::Level::Info;
    switch (type) {
    case QtDebugMsg: level = arete::logging::Level::Debug; break;
    case QtInfoMsg: level = arete::logging::Level::Info; break;
    case QtWarningMsg: level = arete::logging::Level::Warning; break;
    case QtCriticalMsg: level = arete::logging::Level::Error; break;
    case QtFatalMsg: level = arete::logging::Level::Critical; break;
    }
    logger.log(level, message, QString::fromUtf8(context.category ? context.category : "qt"),
               {}, context.file, context.line, context.function);
}

} // namespace

int main(int argc, char* argv[])
{
    // Install early so even QApplication's own warnings (icon themes,
    // multimedia backends) are captured before anything reaches the terminal.
    qInstallMessageHandler(messageHandler);
    // Native stderr from system libraries (VDPAU, ffmpeg) is routed to the
    // void; Arete itself never writes to stderr, everything goes to the log.
    freopen("/dev/null", "w", stderr);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Arete"));
    QApplication::setApplicationDisplayName(QStringLiteral("Arete"));
    QApplication::setOrganizationName(QStringLiteral("Arete"));
    QApplication::setOrganizationDomain(QStringLiteral("arete.local"));

    auto& logger = arete::logging::Logger::instance();
    logger.setLogToFile(true);
    logger.setLogToConsole(false);
    logger.setMinLevel(arete::logging::Level::Debug);

    // Monospace-first design: prefer mono fonts for the whole UI.
    QFont font;
    const QFontDatabase db;
    const QString preferred = QStringLiteral("JetBrains Mono");
    if (!db.families().contains(preferred)) {
        font.setFamily(QStringLiteral("monospace"));
    } else {
        font.setFamily(preferred);
    }
    font.setPointSize(10);
    QApplication::setFont(font);

    arete::Application application;

    // Headless screenshot helper: `arete --screenshot out.png` renders the
    // Home tab after startup and exits. Used for the README demo image.
    const QStringList args = QApplication::arguments();
    const int shotIndex = args.indexOf(QStringLiteral("--screenshot"));
    if (shotIndex >= 0 && shotIndex + 1 < args.size()) {
        const QString shotPath = args.at(shotIndex + 1);
        QTimer::singleShot(2000, [&application, shotPath]() {
            for (QWidget* widget : QApplication::topLevelWidgets()) {
                if (auto* window = qobject_cast<arete::ui::MainWindow*>(widget)) {
                    window->activateModule(QStringLiteral("home"));
                    window->grab().save(shotPath);
                    break;
                }
            }
            QApplication::exit(0);
        });
    }

    const int exitCode = QApplication::exec();
    application.shutdown();
    return exitCode;
}
