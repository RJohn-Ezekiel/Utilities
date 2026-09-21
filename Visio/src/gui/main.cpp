#include "MainWindow.hpp"
#include <visio/ui/Theme.h>

#include "arete/ipc/Ipc.h"
#include "arete/logging/Logger.h"

#include <QApplication>
#include <QTimer>

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

int main(int argc, char* argv[])
{
    // Suppress KDE icon theme warnings
    qputenv("QT_LOGGING_RULES", "kf.*.warning=false");
    qputenv("QT_QPA_PLATFORMTHEME", "");

    QApplication app(argc, argv);
    app.setApplicationName("Visio");
    app.setApplicationVersion("0.2.0");
    app.setOrganizationName("Arete");

    // Internal logging: never print to the terminal during normal operation.
    auto& logger = arete::logging::Logger::instance();
    logger.setMinLevel(arete::logging::Level::Info);
    qInstallMessageHandler(qtMessageHandler);

    // Single-instance + deep-linking (arete://visio/download/<url>)
    QStringList args;
    for (int i = 1; i < argc; ++i) {
        args << QString::fromLocal8Bit(argv[i]);
    }
    static arete::ipc::IpcServer ipc(QStringLiteral("arete-visio"));
    if (!ipc.start()) {
        for (const QString& arg : args) {
            if (arg.startsWith(QStringLiteral("arete://"))) {
                const bool forwarded = arete::ipc::forwardUriToRunningInstance(QStringLiteral("arete-visio"), QUrl(arg));
                Q_UNUSED(forwarded)
                return 0;
            }
        }
    }

    visio::MainWindow window;
    window.show();

    for (const QString& arg : args) {
        if (arg.startsWith(QStringLiteral("arete://"))) {
            const auto action = arete::ipc::parseUri(arg);
            if (action.app == QStringLiteral("visio") && action.command == QStringLiteral("download")
                && action.segments.size() >= 2) {
                window.addDownload(QUrl::fromPercentEncoding(action.segments[1].toUtf8()));
            }
        } else if (arg.startsWith(QStringLiteral("http://")) || arg.startsWith(QStringLiteral("https://"))
                   || arg.startsWith(QStringLiteral("ytdl://"))) {
            window.addDownload(arg);
        }
    }

    QString screenshotPath;
    for (int i = 1; i < argc - 1; ++i) {
        if (qstrcmp(argv[i], "--screenshot") == 0)
            screenshotPath = QString::fromLocal8Bit(argv[i + 1]);
    }
    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(1500, [&] {
            window.grab().save(screenshotPath);
            app.quit();
        });
    }

    return app.exec();
}