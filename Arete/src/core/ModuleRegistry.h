#pragma once

#include "core/Module.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>
#include <memory>
#include <vector>

namespace arete {
namespace app { class AppContext; }

namespace core {

struct ModuleInfo
{
    QString id;
    QString title;
    QString icon;
    bool pinned = false;   // core tab: cannot be removed or reordered
    bool lazy = false;     // heavy module, loaded on first activation
    bool optional = false; // user-toggleable module (vs. core tab)
    QString version;
    QString description;
    bool defaultEnabled = false;
    std::function<Module*(app::AppContext*)> factory;
};

// Central catalog of all modules. Core tabs (Home, Modules, Chronos,
// Settings) are always present; every other module is optional and is
// enabled or disabled from the Modules page. The enabled set and the tab
// order are persisted in the settings file and restored on launch.
class ModuleRegistry : public QObject
{
    Q_OBJECT

public:
    explicit ModuleRegistry(app::AppContext* context, QObject* parent = nullptr);
    ~ModuleRegistry() override;

    void registerModule(const ModuleInfo& info);
    [[nodiscard]] Module* create(const QString& id) const;

    [[nodiscard]] std::vector<ModuleInfo> infos() const;
    [[nodiscard]] ModuleInfo info(const QString& id) const;
    [[nodiscard]] QStringList ids() const;
    [[nodiscard]] bool contains(const QString& id) const;

    // Optional-module management. Pinned core tabs are always enabled.
    [[nodiscard]] bool isEnabled(const QString& id) const;
    void setEnabled(const QString& id, bool enabled);

    // Directory scanned for module metadata (<id>/module.json). Falls back
    // to the app-adjacent modules/ directory when no workspace is known.
    [[nodiscard]] static QString modulesDirectory();

signals:
    void modulesChanged(const QString& id);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace core
} // namespace arete
