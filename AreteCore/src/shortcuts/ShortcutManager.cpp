#include "arete/shortcuts/ShortcutManager.h"
#include <QShortcut>
#include <QWidget>
#include <unordered_map>

namespace arete::shortcuts {

class ShortcutManager::Private
{
public:
    struct Entry
    {
        QKeySequence sequence;
        std::function<void()> callback;
    };
    std::unordered_map<QString, Entry> entries;
};


ShortcutManager::~ShortcutManager() = default;

ShortcutManager::ShortcutManager(QObject* parent)
    : QObject(parent), d(std::make_unique<Private>())
{
}

bool ShortcutManager::registerShortcut(const QString& id, const QKeySequence& sequence,
                                       std::function<void()> callback)
{
    if (id.isEmpty() || sequence.isEmpty()) return false;
    d->entries[id] = Private::Entry{sequence, std::move(callback)};
    emit bindingsChanged();
    return true;
}

bool ShortcutManager::setSequence(const QString& id, const QKeySequence& sequence)
{
    auto it = d->entries.find(id);
    if (it == d->entries.end()) return false;
    it->second.sequence = sequence;
    emit bindingsChanged();
    return true;
}

QKeySequence ShortcutManager::sequence(const QString& id) const
{
    auto it = d->entries.find(id);
    return it != d->entries.end() ? it->second.sequence : QKeySequence();
}

bool ShortcutManager::hasShortcut(const QString& id) const
{
    return d->entries.find(id) != d->entries.end();
}

QStringList ShortcutManager::ids() const
{
    QStringList result;
    for (const auto& [id, entry] : d->entries) {
        result.append(id);
    }
    return result;
}

QHash<QString, QKeySequence> ShortcutManager::allBindings() const
{
    QHash<QString, QKeySequence> result;
    for (const auto& [id, entry] : d->entries) {
        result.insert(id, entry.sequence);
    }
    return result;
}

void ShortcutManager::setBindings(const QHash<QString, QKeySequence>& bindings)
{
    for (auto it = bindings.begin(); it != bindings.end(); ++it) {
        auto entry = d->entries.find(it.key());
        if (entry != d->entries.end()) {
            entry->second.sequence = it.value();
        }
    }
    emit bindingsChanged();
}

void ShortcutManager::setShortcutContext(QWidget* widget)
{
    if (!widget) return;
    const auto shortcuts = widget->findChildren<QShortcut*>();
    for (QShortcut* s : shortcuts) {
        s->setContext(Qt::WindowShortcut);
    }
}

} // namespace arete::shortcuts