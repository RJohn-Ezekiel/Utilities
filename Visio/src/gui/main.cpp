#include "MainWindow.hpp"
#include <visio/ui/Theme.h>

#include <QApplication>
#include <QTimer>

int main(int argc, char* argv[])
{
    // Suppress KDE icon theme warnings
    qputenv("QT_LOGGING_RULES", "kf.*.warning=false");
    qputenv("QT_QPA_PLATFORMTHEME", "");

    QApplication app(argc, argv);
    app.setApplicationName("Visio");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Visio");

    visio::MainWindow window;
    window.show();

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
