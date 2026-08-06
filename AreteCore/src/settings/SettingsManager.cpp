#include "arete/settings/SettingsManager.h"
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QCoreApplication>
#include <unordered_map>
#include <mutex>

namespace arete::settings {

class SettingsManager::Private
{
public:
    explicit Private(const QString& org, const QString& app, Scope scope, Format format)
        : organization(org), application(app), scope(scope), format(format)
    {
        rebuildSettings();
    }

    void rebuildSettings()
    {
        QSettings::Format qtFormat = (format == Format::Ini) ? QSettings::IniFormat : QSettings::NativeFormat;
        QSettings::Scope qtScope = (scope == Scope::System) ? QSettings::SystemScope : QSettings::UserScope;

        if (portableMode && !configPath.isEmpty()) {
            settings = std::make_unique<QSettings>(configPath, qtFormat);
        } else {
            settings = std::make_unique<QSettings>(qtFormat, qtScope, organization, application);
        }
    }

    QString organization;
    QString application;
    Scope scope;
    Format format;
    bool portableMode = false;
    QString configPath;
    std::unique_ptr<QSettings> settings;
    QString currentGroup;
    std::unordered_map<QString, std::vector<std::function<void(const QString&, const QVariant&, const QVariant&)>>> changeCallbacks;
    std::unordered_map<QString, std::function<bool(const QVariant&)>> validators;
    std::mutex callbacksMutex;
};


SettingsManager::~SettingsManager() = default;

SettingsManager::SettingsManager(const QString& organization, const QString& application,
                                 Scope scope, Format format, QObject* parent)
    : QObject(parent), d(std::make_unique<Private>(organization, application, scope, format))
{
}

QVariant SettingsManager::value(const QString& key, const QVariant& defaultValue) const
{
    const QString fullKey = d->currentGroup.isEmpty() ? key : d->currentGroup + "/" + key;
    return d->settings->value(fullKey, defaultValue);
}

void SettingsManager::setValue(const QString& key, const QVariant& value)
{
    const QString fullKey = d->currentGroup.isEmpty() ? key : d->currentGroup + "/" + key;

    // Validate if validator exists
    auto it = d->validators.find(fullKey);
    if (it != d->validators.end() && !it->second(value)) {
        return; // Validation failed, don't set
    }

    const QVariant oldValue = d->settings->value(fullKey);
    if (oldValue != value) {
        d->settings->setValue(fullKey, value);

        // Notify callbacks
        std::lock_guard<std::mutex> lock(d->callbacksMutex);
        auto cbIt = d->changeCallbacks.find(fullKey);
        if (cbIt != d->changeCallbacks.end()) {
            for (const auto& callback : cbIt->second) {
                callback(fullKey, oldValue, value);
            }
        }

        emit valueChanged(fullKey, value);
    }
}

void SettingsManager::beginGroup(const QString& prefix)
{
    if (!d->currentGroup.isEmpty()) {
        d->currentGroup += "/";
    }
    d->currentGroup += prefix;
    d->settings->beginGroup(prefix);
}

void SettingsManager::endGroup()
{
    d->settings->endGroup();
    int idx = d->currentGroup.lastIndexOf('/');
    if (idx != -1) {
        d->currentGroup = d->currentGroup.left(idx);
    } else {
        d->currentGroup.clear();
    }
}

QString SettingsManager::group() const
{
    return d->currentGroup;
}

int SettingsManager::beginReadArray(const QString& prefix)
{
    return d->settings->beginReadArray(prefix);
}

void SettingsManager::endArray()
{
    d->settings->endArray();
}

void SettingsManager::beginWriteArray(const QString& prefix, int size)
{
    d->settings->beginWriteArray(prefix, size);
}

void SettingsManager::setArrayIndex(int index)
{
    d->settings->setArrayIndex(index);
}

QStringList SettingsManager::allKeys() const
{
    return d->settings->allKeys();
}

QStringList SettingsManager::childGroups() const
{
    return d->settings->childGroups();
}

QStringList SettingsManager::childKeys() const
{
    return d->settings->childKeys();
}

bool SettingsManager::contains(const QString& key) const
{
    const QString fullKey = d->currentGroup.isEmpty() ? key : d->currentGroup + "/" + key;
    return d->settings->contains(fullKey);
}

void SettingsManager::remove(const QString& key)
{
    const QString fullKey = d->currentGroup.isEmpty() ? key : d->currentGroup + "/" + key;
    d->settings->remove(fullKey);
}

void SettingsManager::clear()
{
    d->settings->clear();
}

void SettingsManager::sync()
{
    d->settings->sync();
    emit settingsSaved();
}

QSettings::Status SettingsManager::status() const
{
    return d->settings->status();
}

void SettingsManager::onValueChanged(const QString& key, ChangeCallback callback)
{
    const QString fullKey = d->currentGroup.isEmpty() ? key : d->currentGroup + "/" + key;
    std::lock_guard<std::mutex> lock(d->callbacksMutex);
    d->changeCallbacks[fullKey].push_back(std::move(callback));
}

void SettingsManager::removeChangeCallback(const QString& key)
{
    const QString fullKey = d->currentGroup.isEmpty() ? key : d->currentGroup + "/" + key;
    std::lock_guard<std::mutex> lock(d->callbacksMutex);
    d->changeCallbacks.erase(fullKey);
}

void SettingsManager::setPortableMode(bool enabled)
{
    if (d->portableMode != enabled) {
        d->portableMode = enabled;
        d->rebuildSettings();
        emit settingsLoaded();
    }
}

bool SettingsManager::isPortableMode() const
{
    return d->portableMode;
}

void SettingsManager::setConfigPath(const QString& path)
{
    d->configPath = path;
    if (d->portableMode) {
        d->rebuildSettings();
    }
}

bool SettingsManager::exportToFile(const QString& filePath) const
{
    QJsonObject root;
    const QStringList keys = d->settings->allKeys();
    for (const QString& key : keys) {
        root[key] = QJsonValue::fromVariant(d->settings->value(key));
    }

    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool SettingsManager::importFromFile(const QString& filePath, bool merge)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    if (!merge) {
        d->settings->clear();
    }

    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        d->settings->setValue(it.key(), it.value().toVariant());
    }

    d->settings->sync();
    emit settingsLoaded();
    return true;
}

void SettingsManager::setValidator(const QString& key, Validator validator)
{
    const QString fullKey = d->currentGroup.isEmpty() ? key : d->currentGroup + "/" + key;
    d->validators[fullKey] = std::move(validator);
}

bool SettingsManager::validate(const QString& key) const
{
    const QString fullKey = d->currentGroup.isEmpty() ? key : d->currentGroup + "/" + key;
    auto it = d->validators.find(fullKey);
    if (it != d->validators.end()) {
        return it->second(d->settings->value(fullKey));
    }
    return true;
}

} // namespace arete::settings