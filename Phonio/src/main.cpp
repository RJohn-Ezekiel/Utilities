#include "app/App.h"
#include "library/LibraryManager.h"
#include "player/PlaybackController.h"
#include "settings/SettingsManager.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include "arete/ipc/Ipc.h"
#include "arete/logging/Logger.h"

#include <QApplication>
#include <QTimer>
#include <QWidget>

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
    // Keep the FFmpeg media backend quiet (no "Input #0, mp3..." dumps).
    qputenv("QT_LOGGING_RULES", "qt.multimedia.ffmpeg=false;kf.*.warning=false");

    QString screenshotPath;
    bool pageNowPlaying = false;
    QStringList args;
    for (int i = 1; i < argc; ++i) {
        args << QString::fromLocal8Bit(argv[i]);
        if (i < argc - 1 && qstrcmp(argv[i], "--screenshot") == 0)
            screenshotPath = QString::fromLocal8Bit(argv[i + 1]);
        if (qstrcmp(argv[i], "--page-nowplaying") == 0)
            pageNowPlaying = true;
    }

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Phonio"));
    QApplication::setOrganizationName(QStringLiteral("Arete"));
    QApplication::setApplicationVersion(QStringLiteral("0.2.0"));

    // Internal logging: never print to the terminal during normal operation.
    auto& logger = arete::logging::Logger::instance();
    logger.setMinLevel(arete::logging::Level::Info);
    qInstallMessageHandler(qtMessageHandler);

    // Single-instance + deep-linking (arete://phonio/playlist/Focus)
    static arete::ipc::IpcServer ipc(QStringLiteral("arete-phonio"));
    if (!ipc.start()) {
        for (const QString& arg : args) {
            if (arg.startsWith(QStringLiteral("arete://"))) {
                const bool forwarded =
                    arete::ipc::forwardUriToRunningInstance(QStringLiteral("arete-phonio"), QUrl(arg));
                Q_UNUSED(forwarded);
                return 0;
            }
        }
    }

    phonio::SettingsManager settings;
    phonio::Theme::apply(app, settings.accentColor());

    phonio::App phonioApp(app);

    // Handle deep links
    for (const QString& arg : args) {
        if (arg.startsWith(QStringLiteral("arete://"))) {
            const auto action = arete::ipc::parseUri(arg);
            if (action.app == QStringLiteral("phonio") && action.command == QStringLiteral("playlist")
                && action.segments.size() >= 2) {
                phonioApp.mainWindow()->openPlaylistByName(action.segments[1]);
            }
        }
    }

    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(2500, [&] {
            if (pageNowPlaying) {
                if (auto* lib = phonioApp.library(); lib && lib->trackCount() > 0)
                    phonioApp.controller()->playTrack(lib->tracks().first());
                phonioApp.mainWindow()->showNowPlaying();
            }
            QTimer::singleShot(300, [&] {
                phonioApp.mainWindow()->grab().save(screenshotPath);
                app.quit();
            });
        });
    }

    return phonioApp.run();
}
