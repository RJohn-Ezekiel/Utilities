#include "app/Application.h"
#include "app/AppContext.h"
#include "ui/MainWindow.h"
#include "arete/theme/Theme.h"
#include "arete/logging/Logger.h"

#include <QApplication>
#include <QMainWindow>

using arete::logging::Logger;

namespace arete {

class Application::Private
{
public:
    std::unique_ptr<app::AppContext> context;
    std::unique_ptr<ui::MainWindow> window;
};

Application::Application(QObject* parent)
    : QObject(parent), d(std::make_unique<Private>())
{
    applyTheme();

    d->context = std::make_unique<app::AppContext>(this);
    d->window = std::make_unique<ui::MainWindow>(d->context.get());
    d->context->setMainWindow(d->window.get());

    restoreSession();

    d->window->show();
    Logger::instance().info(QStringLiteral("Arete started"), QStringLiteral("app"));
}

Application::~Application()
{
    shutdown();
}

void Application::applyTheme()
{
    qApp->setStyleSheet(arete::theme::appStyleSheet());
}

void Application::restoreSession()
{
    d->window->restoreSession();
}

void Application::shutdown()
{
    if (d->window) {
        d->window->saveSession();
    }
}

} // namespace arete
