#include "arete/dialogs/DiagnosticsDialog.h"
#include "arete/logging/Logger.h"
#include "arete/notifications/NotificationCenter.h"
#include "arete/widgets/LogsPage.h"
#include "arete/widgets/NotificationsPanel.h"
#include "arete/theme/Theme.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QApplication>

namespace arete::dialogs {

class DiagnosticsDialog::Private
{
public:
    widgets::LogsPage* logsPage = nullptr;
    widgets::NotificationsPanel* notificationsPanel = nullptr;
};

DiagnosticsDialog::DiagnosticsDialog(QWidget* parent)
    : QDialog(parent)
    , d(std::make_unique<Private>())
{
    setWindowTitle(QStringLiteral("Diagnostics"));
    setMinimumSize(720, 480);
    resize(820, 540);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);

    auto* tabs = new QTabWidget(this);
    tabs->setDocumentMode(true);

    d->logsPage = new widgets::LogsPage(tabs);
    d->logsPage->setLogger(&logging::Logger::instance());
    tabs->addTab(d->logsPage, QStringLiteral("Logs"));

    d->notificationsPanel = new widgets::NotificationsPanel(tabs);
    d->notificationsPanel->setNotificationCenter(
        &notifications::NotificationCenter::instance());
    tabs->addTab(d->notificationsPanel, QStringLiteral("Notifications"));

    root->addWidget(tabs);

    if (d->logsPage && d->notificationsPanel) {
        // Keep the tab count visible in the title.
        connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
            Q_UNUSED(index);
            const int unread = d->notificationsPanel->unreadCount();
            setWindowTitle(unread > 0
                ? QStringLiteral("Diagnostics (%1 unread)").arg(unread)
                : QStringLiteral("Diagnostics"));
        });
    }

    setStyleSheet(arete::theme::appStyleSheet());
}

DiagnosticsDialog::~DiagnosticsDialog() = default;

widgets::LogsPage* DiagnosticsDialog::logsPage() const
{
    return d->logsPage;
}

widgets::NotificationsPanel* DiagnosticsDialog::notificationsPanel() const
{
    return d->notificationsPanel;
}

void DiagnosticsDialog::openDialog(QWidget* parent)
{
    auto* dialog = new DiagnosticsDialog(parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

} // namespace arete::dialogs
