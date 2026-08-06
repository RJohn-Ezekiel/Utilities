#pragma once

#include "core/Module.h"

#include <QLabel>
#include <QStackedWidget>

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Chronos tab: the vendored focus-timer application embedded as the module
// content. Loaded lazily on first activation to keep startup fast.
class ChronosModule : public core::Module
{
    Q_OBJECT

public:
    explicit ChronosModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("chronos"); }
    QString moduleTitle() const override { return QStringLiteral("Chronos"); }
    QString moduleIcon() const override { return QString(); }
    bool lazyLoads() const override { return true; }

protected:
    void lazyLoad() override;

private:
    QLabel* m_placeholder;
};

} // namespace ui
} // namespace arete
