#pragma once

#include <QWidget>
#include <QString>

namespace arete {
namespace app { class AppContext; }

namespace core {

// Base class for every Arete module (Home, Today, Projects, ...).
// Modules are decoupled from each other; they communicate through
// AppContext services and repository signals.
class Module : public QWidget
{
    Q_OBJECT

public:
    explicit Module(app::AppContext* context, QWidget* parent = nullptr);

    [[nodiscard]] app::AppContext* context() const { return m_context; }

    [[nodiscard]] virtual QString moduleId() const = 0;
    [[nodiscard]] virtual QString moduleTitle() const = 0;
    [[nodiscard]] virtual QString moduleIcon() const = 0;

    // Heavy modules load data lazily on first activation instead of at
    // startup, keeping cold start under two seconds.
    [[nodiscard]] virtual bool lazyLoads() const { return false; }

    // Called by the tab manager every time the module becomes visible.
    void activate()
    {
        ensureLoaded();
        onActivated();
    }

    // Modules may override to run refresh logic on each activation.
    virtual void onActivated() {}

    // Convenience: refresh one of the module's views.
    virtual void refreshView() {}

    // Hook for the palette command "go <module id>".
    virtual void handleCommand(const QString& /*command*/) {}

protected:
    // Called once before the first activation; populates widgets.
    virtual void lazyLoad() {}

    void ensureLoaded()
    {
        if (!m_loaded) {
            m_loaded = true;
            lazyLoad();
        }
    }

private:
    app::AppContext* m_context;
    bool m_loaded = false;
};

} // namespace core
} // namespace arete
