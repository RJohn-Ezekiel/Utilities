#pragma once

#include <QObject>
#include <QKeySequence>
#include <QHash>
#include <QStringList>
#include <memory>

namespace arete::shortcuts {

class ShortcutManager : public QObject
{
    Q_OBJECT

public:
    explicit ShortcutManager(QObject* parent = nullptr);
    ~ShortcutManager() override;

    // Register a shortcut with an id. Callback is invoked when triggered.
    // Returns true if registered successfully.
    bool registerShortcut(const QString& id, const QKeySequence& sequence,
                          std::function<void()> callback);

    // Set/change the key sequence for an existing shortcut
    bool setSequence(const QString& id, const QKeySequence& sequence);

    // Lookup
    [[nodiscard]] QKeySequence sequence(const QString& id) const;
    [[nodiscard]] bool hasShortcut(const QString& id) const;

    // List all registered ids
    [[nodiscard]] QStringList ids() const;

    // Export/import bindings (for Settings pages)
    [[nodiscard]] QHash<QString, QKeySequence> allBindings() const;
    void setBindings(const QHash<QString, QKeySequence>& bindings);

    // Find the parent widget that should own the shortcuts
    static void setShortcutContext(QWidget* widget);

signals:
    void shortcutTriggered(const QString& id);
    void bindingsChanged();

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace arete::shortcuts