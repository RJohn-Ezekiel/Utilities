#include "MainWindow.hpp"
#include <visio/ui/Theme.h>

#include <QApplication>

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

    return app.exec();
}
