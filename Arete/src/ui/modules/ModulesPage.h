#pragma once

#include "core/Module.h"

#include <QWidget>

class QLabel;
class QListWidget;
class QVBoxLayout;

namespace arete {
namespace app { class AppContext; }

namespace ui {

// The Modules tab — the control centre of Arete's modular architecture.
// Core tabs (Home, Modules, Chronos, Settings) are permanent; every other
// module is listed here and can be enabled or disabled. Enabled modules
// appear as tabs, disabled ones do not, and both the visible set and the
// tab order are remembered across launches.
class ModulesModule : public core::Module
{
    Q_OBJECT

public:
    explicit ModulesModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("modules"); }
    QString moduleTitle() const override { return QStringLiteral("Modules"); }
    QString moduleIcon() const override { return QString(); }
    void onActivated() override;
    void refreshView() override;

private:
    void rebuild();

    QListWidget* m_list;
    QLabel* m_summary;
    QVBoxLayout* m_layout;
};

} // namespace ui
} // namespace arete