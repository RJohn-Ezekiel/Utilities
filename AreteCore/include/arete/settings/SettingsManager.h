#pragma once

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <QString>
#include <QStringList>
#include <functional>
#include <memory>

namespace arete::settings {

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    enum class Scope { User, System };
    enum class Format { Native, Ini };

    explicit SettingsManager(const QString& organization, const QString& application,
                             Scope scope = Scope::User, Format format = Format::Native,
                             QObject* parent = nullptr);
    ~SettingsManager() override;

    // Non-copyable, movable
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    SettingsManager(SettingsManager&&) = default;
    SettingsManager& operator=(SettingsManager&&) = default;

    // Value access
    [[nodiscard]] QVariant value(const QString& key, const QVariant& defaultValue = {}) const;
    void setValue(const QString& key, const QVariant& value);

    // Typed accessors
    template <typename T>
    [[nodiscard]] T get(const QString& key, const T& defaultValue = T{}) const
    {
        return value(key, QVariant(defaultValue)).template value<T>();
    }

    template <typename T>
    void set(const QString& key, const T& value)
    {
        setValue(key, QVariant::fromValue(value));
    }

    // Group management
    void beginGroup(const QString& prefix);
    void endGroup();
    [[nodiscard]] QString group() const;

    // Array handling
    int beginReadArray(const QString& prefix);
    void endArray();
    void beginWriteArray(const QString& prefix, int size = -1);
    void setArrayIndex(int index);

    // Key management
    [[nodiscard]] QStringList allKeys() const;
    [[nodiscard]] QStringList childGroups() const;
    [[nodiscard]] QStringList childKeys() const;
    bool contains(const QString& key) const;
    void remove(const QString& key);
    void clear();

    // Sync
    void sync();
    [[nodiscard]] QSettings::Status status() const;

    // Change notification
    using ChangeCallback = std::function<void(const QString& key, const QVariant& oldValue, const QVariant& newValue)>;
    void onValueChanged(const QString& key, ChangeCallback callback);
    void removeChangeCallback(const QString& key);

    // Portable mode support
    void setPortableMode(bool enabled);
    [[nodiscard]] bool isPortableMode() const;
    void setConfigPath(const QString& path);

    // Import/Export
    [[nodiscard]] bool exportToFile(const QString& filePath) const;
    [[nodiscard]] bool importFromFile(const QString& filePath, bool merge = true);

    // Validation
    using Validator = std::function<bool(const QVariant&)>;
    void setValidator(const QString& key, Validator validator);
    [[nodiscard]] bool validate(const QString& key) const;

signals:
    void valueChanged(const QString& key, const QVariant& value);
    void settingsSaved();
    void settingsLoaded();

private:
    class Private;
    std::unique_ptr<Private> d;
};

// Convenience macro for defining settings keys with type safety
#define ARETE_SETTING_KEY(T, name, defaultValue) \
    inline const char* name##_key() { return #name; } \
    inline T name##_default() { return defaultValue; }

} // namespace arete::settings