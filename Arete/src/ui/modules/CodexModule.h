#pragma once

#include "core/Module.h"

#include <QLabel>

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Codex tab: the vendored markdown vault editor embedded as the module
// content. Loaded lazily on first activation.
class CodexModule : public core::Module
{
    Q_OBJECT

public:
    explicit CodexModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("codex"); }
    QString moduleTitle() const override { return QStringLiteral("Codex"); }
    QString moduleIcon() const override { return QString(); }
    bool lazyLoads() const override { return true; }

protected:
    void lazyLoad() override;

private:
    QLabel* m_placeholder;
};

} // namespace ui
} // namespace arete
