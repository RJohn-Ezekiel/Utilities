#include "core/ModuleRegistry.h"
#include "app/AppContext.h"
#include "arete/settings/SettingsManager.h"
#include "arete/update/UpdateService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace arete::core {

class ModuleRegistry::Private
{
public:
    explicit Private(app::AppContext* ctx) : context(ctx) {}
    app::AppContext* context;
    std::vector<ModuleInfo> infos;
};

ModuleRegistry::ModuleRegistry(app::AppContext* context, QObject* parent)
    : QObject(parent), d(std::make_unique<Private>(context))
{
}

ModuleRegistry::~ModuleRegistry() = default;

void ModuleRegistry::registerModule(const ModuleInfo& info)
{
    ModuleInfo merged = info;

    // Merge metadata from modules/<id>/module.json when present.
    const QString metaFile = modulesDirectory() + QLatin1Char('/') + info.id
        + QStringLiteral("/module.json");
    QFile file(metaFile);
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        if (!root.value(QStringLiteral("name")).toString().isEmpty()) {
            merged.title = root.value(QStringLiteral("name")).toString();
        }
        merged.version = root.value(QStringLiteral("version")).toString();
        merged.description = root.value(QStringLiteral("description")).toString();
        merged.defaultEnabled = root.value(QStringLiteral("defaultEnabled")).toBool(false);
    }

    // Seed the enabled state from a pre-existing tab order so upgrading
    // keeps the user's current tabs; fresh profiles start with the core
    // four tabs only.
    if (merged.optional && !d->context->settings()->contains(QStringLiteral("modules/") + info.id + QStringLiteral("/enabled"))) {
        const QStringList savedOrder = d->context->settings()
            ->get(QStringLiteral("ui/tabOrder"), QString()).split(QLatin1Char(','), Qt::SkipEmptyParts);
        const bool wasVisible = savedOrder.contains(info.id);
        d->context->settings()->set(QStringLiteral("modules/") + info.id + QStringLiteral("/enabled"),
                                    wasVisible ? true : merged.defaultEnabled);
    }

    d->infos.push_back(merged);
    std::sort(d->infos.begin(), d->infos.end(),
              [](const ModuleInfo& a, const ModuleInfo& b) {
                  if (a.pinned != b.pinned) return a.pinned;
                  return a.title < b.title;
              });
}

Module* ModuleRegistry::create(const QString& id) const
{
    for (const ModuleInfo& info : d->infos) {
        if (info.id == id) {
            return info.factory(d->context);
        }
    }
    return nullptr;
}

std::vector<ModuleInfo> ModuleRegistry::infos() const
{
    return d->infos;
}

ModuleInfo ModuleRegistry::info(const QString& id) const
{
    for (const ModuleInfo& info : d->infos) {
        if (info.id == id) return info;
    }
    return {};
}

QStringList ModuleRegistry::ids() const
{
    QStringList result;
    for (const ModuleInfo& info : d->infos) {
        result.append(info.id);
    }
    return result;
}

bool ModuleRegistry::contains(const QString& id) const
{
    return std::any_of(d->infos.begin(), d->infos.end(),
                       [&id](const ModuleInfo& info) { return info.id == id; });
}

bool ModuleRegistry::isEnabled(const QString& id) const
{
    const ModuleInfo info = this->info(id);
    if (info.pinned || !info.optional) return true;
    return d->context->settings()->get(QStringLiteral("modules/") + id + QStringLiteral("/enabled"),
                                       info.defaultEnabled);
}

void ModuleRegistry::setEnabled(const QString& id, bool enabled)
{
    d->context->settings()->set(QStringLiteral("modules/") + id + QStringLiteral("/enabled"), enabled);
    d->context->settings()->sync();
    emit modulesChanged(id);
}

QString ModuleRegistry::modulesDirectory()
{
    // The modules/ folder ships with the app. It may live in any of these
    // locations depending on how Arete was installed or launched:
    const QString workspace = arete::update::UpdateService::workspaceRoot();
    const QStringList candidates = {
        workspace + QStringLiteral("/modules"),            // shared workspace
        workspace + QStringLiteral("/Arete/modules"),          // dev: under app source
        QCoreApplication::applicationDirPath() + QStringLiteral("/modules"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../modules"),
    };
    for (const QString& dir : candidates) {
        if (QDir(dir).exists()) return dir;
    }
    return candidates.first();
}

} // namespace arete::core
