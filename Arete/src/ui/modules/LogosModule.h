#pragma once

#include "core/Module.h"

#include <QLabel>

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Logos tab: the vendored Bible reader embedded as the module content.
// Loaded lazily on first activation.
class LogosModule : public core::Module
{
    Q_OBJECT

public:
    explicit LogosModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("logos"); }
    QString moduleTitle() const override { return QStringLiteral("Logos"); }
    QString moduleIcon() const override { return QString(); }
    bool lazyLoads() const override { return true; }
    void handleCommand(const QString& command) override;

protected:
    void lazyLoad() override;

private:
    QLabel* m_placeholder;
    QLabel* m_verseLabel;
};

} // namespace ui
} // namespace arete
