#pragma once

#include "core/Module.h"

#include <QLabel>

namespace arete {
namespace app { class AppContext; }

namespace ui {

// Phonio tab: the vendored music player embedded as the module content.
// Loaded lazily on first activation.
class PhonioModule : public core::Module
{
    Q_OBJECT

public:
    explicit PhonioModule(app::AppContext* context, QWidget* parent = nullptr);

    QString moduleId() const override { return QStringLiteral("phonio"); }
    QString moduleTitle() const override { return QStringLiteral("Phonio"); }
    QString moduleIcon() const override { return QString(); }
    bool lazyLoads() const override { return true; }

protected:
    void lazyLoad() override;

private:
    QLabel* m_placeholder;
};

} // namespace ui
} // namespace arete
