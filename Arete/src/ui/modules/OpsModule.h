#pragma once

#include "core/Module.h"
#include "core/Icons.h"

#include <QWidget>

class QLabel;
class QTableWidget;
class QVBoxLayout;
class QPushButton;

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Ops tab: a chronological activity feed in the style of the crona-main
// "ops log" view — Time / Entity / Action / Target rows spanning focus
// sessions and completed tasks.
class OpsModule : public core::Module
{
    Q_OBJECT

public:
    explicit OpsModule(app::AppContext* context, QWidget* parent = nullptr);

    [[nodiscard]] QString moduleId() const override { return QStringLiteral("ops"); }
    [[nodiscard]] QString moduleTitle() const override { return QStringLiteral("Ops"); }
    [[nodiscard]] QString moduleIcon() const override { return core::icons::Ops; }

    void onActivated() override;
    void refreshView() override;

private:
    void rebuildTable();

    QTableWidget* m_table = nullptr;
    QLabel* m_emptyLabel = nullptr;
    QVBoxLayout* m_layout = nullptr;
};

} // namespace ui
} // namespace arete
