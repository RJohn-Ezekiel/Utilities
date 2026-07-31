#include "app/App.h"
#include "library/LibraryManager.h"
#include "player/PlaybackController.h"
#include "settings/SettingsManager.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QTimer>
#include <QWidget>

int main(int argc, char* argv[])
{
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
    QApplication::setOrganizationName(QStringLiteral("Phonio"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    phonio::SettingsManager settings;
    phonio::Theme::apply(app, settings.accentColor());

    phonio::App phonioApp(app);

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
