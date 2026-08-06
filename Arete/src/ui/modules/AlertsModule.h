#pragma once

#include "core/Module.h"
#include "arete/notifications/NotificationCenter.h"

#include <QWidget>

class QLabel;
class QVBoxLayout;
class QPushButton;

namespace arete::notifications { class NotificationModel; }

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Alerts tab: history of every notification Arete posted (focus session
// started, reminders, errors, ...). Fed directly by the notification
// center, so anything that shows a toast lands here too.
class AlertsModule : public core::Module
{
    Q_OBJECT

public:
    explicit AlertsModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("alerts"); }
    QString moduleTitle() const override { return QStringLiteral("Alerts"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;

private:
    void rebuildList();
    QString typeName(arete::notifications::Type type) const;
    QString typeColor(arete::notifications::Type type) const;

    arete::notifications::NotificationModel* m_model;
    QWidget* m_listHost;
    QVBoxLayout* m_listLayout;
    QLabel* m_emptyLabel;
};

} // namespace ui
} // namespace arete