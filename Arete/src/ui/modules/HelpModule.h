#pragma once

#include "core/Module.h"
#include "core/Icons.h"

#include <QWidget>

class QLabel;
class QVBoxLayout;

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Help tab: keyboard shortcuts, tab map, and pointers to logs, alerts,
// and the bug tracker — modelled on the crona-main support view.
class HelpModule : public core::Module
{
    Q_OBJECT

public:
    explicit HelpModule(app::AppContext* context, QWidget* parent = nullptr);

    [[nodiscard]] QString moduleId() const override { return QStringLiteral("help"); }
    [[nodiscard]] QString moduleTitle() const override { return QStringLiteral("Help"); }
    [[nodiscard]] QString moduleIcon() const override { return core::icons::Help; }

private:
    QLabel* addSection(QVBoxLayout* layout, const QString& title);
    void addRow(QVBoxLayout* layout, const QString& key, const QString& value);
    void addActionRow(QVBoxLayout* layout, const QString& label, std::function<void()> action);
};

} // namespace ui
} // namespace arete
