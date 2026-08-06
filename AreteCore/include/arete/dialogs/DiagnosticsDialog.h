#pragma once

#include <QDialog>
#include <memory>

namespace arete::notifications {
class NotificationCenter;
}

namespace arete::widgets {
class LogsPage;
class NotificationsPanel;
}

namespace arete::dialogs {

// A diagnostic dialog with two tabs: the application log viewer and the
// notification center panel. Shared by all Arete applications.
class DiagnosticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiagnosticsDialog(QWidget* parent = nullptr);
    ~DiagnosticsDialog() override;

    static void openDialog(QWidget* parent = nullptr);

    [[nodiscard]] widgets::LogsPage* logsPage() const;
    [[nodiscard]] widgets::NotificationsPanel* notificationsPanel() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace arete::dialogs
