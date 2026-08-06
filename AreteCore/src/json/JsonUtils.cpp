#include "arete/json/JsonUtils.h"
#include <QSaveFile>
#include <QTimer>
#include <QFileSystemWatcher>
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace arete::json {

QString JsonError::toString() const
{
    QString result;
    switch (code) {
        case Code::None: result = QStringLiteral("No error"); break;
        case Code::FileNotFound: result = QStringLiteral("File not found"); break;
        case Code::PermissionDenied: result = QStringLiteral("Permission denied"); break;
        case Code::InvalidJson: result = QStringLiteral("Invalid JSON"); break;
        case Code::TypeMismatch: result = QStringLiteral("Type mismatch"); break;
        case Code::MissingKey: result = QStringLiteral("Missing key"); break;
        case Code::SchemaValidationFailed: result = QStringLiteral("Schema validation failed"); break;
        case Code::WriteFailed: result = QStringLiteral("Write failed"); break;
    }
    if (!filePath.isEmpty()) {
        result += QStringLiteral(" in %1").arg(filePath);
    }
    if (line > 0 || column > 0) {
        result += QStringLiteral(" at %1:%2").arg(line).arg(column);
    }
    if (!message.isEmpty()) {
        result += QStringLiteral(": %1").arg(message);
    }
    return result;
}

// ============================================================================
// File Operations
// ============================================================================

namespace {

QString documentTypeName(const QJsonDocument& doc)
{
    if (doc.isObject()) return QStringLiteral("object");
    if (doc.isArray()) return QStringLiteral("array");
    if (doc.isNull()) return QStringLiteral("null");
    return QStringLiteral("undefined");
}

} // namespace

JsonResult loadFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        return JsonResult::fail(JsonError::make(JsonError::Code::FileNotFound,
                                 QStringLiteral("File does not exist: %1").arg(filePath),
                                 filePath));
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return JsonResult::fail(JsonError::make(JsonError::Code::PermissionDenied,
                                 QStringLiteral("Cannot open file for reading: %1").arg(file.errorString()),
                                 filePath));
    }

    const QByteArray data = file.readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return JsonResult::fail(JsonError::make(JsonError::Code::InvalidJson,
                                 parseError.errorString(),
                                 filePath,
                                 parseError.offset, 0));
    }
    return JsonResult::ok(std::move(doc));
}

JsonObjectResult loadObject(const QString& filePath)
{
    auto result = loadFile(filePath);
    if (result.hasError()) return JsonObjectResult::fail(result.error());
    if (!result.value().isObject()) {
        return JsonObjectResult::fail(JsonError::make(JsonError::Code::TypeMismatch,
                                       QStringLiteral("Expected JSON object, got %1").arg(documentTypeName(result.value())),
                                       filePath));
    }
    return JsonObjectResult::ok(result.value().object());
}

JsonArrayResult loadArray(const QString& filePath)
{
    auto result = loadFile(filePath);
    if (result.hasError()) return JsonArrayResult::fail(result.error());
    if (!result.value().isArray()) {
        return JsonArrayResult::fail(JsonError::make(JsonError::Code::TypeMismatch,
                                       QStringLiteral("Expected JSON array, got %1").arg(documentTypeName(result.value())),
                                       filePath));
    }
    return JsonArrayResult::ok(result.value().array());
}

BoolResult saveFile(const QString& filePath, const QJsonDocument& doc,
                    bool createBackup, bool prettyPrint)
{
    QFileInfo info(filePath);
    QDir().mkpath(info.absolutePath());

    // Create backup of existing file
    if (createBackup && QFile::exists(filePath)) {
        QString backupPath = filePath + QStringLiteral(".bak");
        QFile::remove(backupPath);
        QFile::copy(filePath, backupPath);
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return BoolResult::fail(JsonError::make(JsonError::Code::PermissionDenied,
                                 QStringLiteral("Cannot open file for writing: %1").arg(file.errorString()),
                                 filePath));
    }

    const QByteArray data = doc.toJson(prettyPrint ? QJsonDocument::Indented : QJsonDocument::Compact);
    if (file.write(data) != data.size()) {
        return BoolResult::fail(JsonError::make(JsonError::Code::WriteFailed,
                                 QStringLiteral("Failed to write file: %1").arg(file.errorString()),
                                 filePath));
    }

    if (!file.commit()) {
        return BoolResult::fail(JsonError::make(JsonError::Code::WriteFailed,
                                 QStringLiteral("Failed to commit file: %1").arg(file.errorString()),
                                 filePath));
    }

    return BoolResult::ok(true);
}

BoolResult saveObject(const QString& filePath, const QJsonObject& obj,
                      bool createBackup, bool prettyPrint)
{
    return saveFile(filePath, QJsonDocument(obj), createBackup, prettyPrint);
}

BoolResult saveArray(const QString& filePath, const QJsonArray& arr,
                     bool createBackup, bool prettyPrint)
{
    return saveFile(filePath, QJsonDocument(arr), createBackup, prettyPrint);
}

// ============================================================================
// Validation
// ============================================================================

Schema& Schema::required(const QString& key, QJsonValue::Type type, const QString& description)
{
    rules.append({key, type, true, nullptr, description});
    return *this;
}

Schema& Schema::optional(const QString& key, QJsonValue::Type type, const QString& description)
{
    rules.append({key, type, false, nullptr, description});
    return *this;
}

Schema& Schema::custom(const QString& key, std::function<bool(const QJsonValue&)> validator,
                       const QString& description)
{
    rules.append({key, QJsonValue::Undefined, false, std::move(validator), description});
    return *this;
}

ValidationResult validate(const QJsonObject& object, const Schema& schema)
{
    ValidationResult result;
    for (const auto& rule : schema.rules) {
        if (rule.required && !object.contains(rule.key)) {
            result.valid = false;
            result.errors.append(JsonError::make(JsonError::Code::MissingKey,
                                  QStringLiteral("Required key '%1' is missing%2")
                                      .arg(rule.key, rule.description.isEmpty() ? QString() : " (" + rule.description + ")") ));
            continue;
        }

        if (!object.contains(rule.key)) continue;

        const QJsonValue value = object.value(rule.key);

        if (rule.customValidator) {
            if (!rule.customValidator(value)) {
                result.valid = false;
                result.errors.append(JsonError::make(JsonError::Code::SchemaValidationFailed,
                                      QStringLiteral("Custom validation failed for '%1'%2")
                                          .arg(rule.key, rule.description.isEmpty() ? QString() : " (" + rule.description + ")") ));
            }
            continue;
        }

        if (rule.expectedType != QJsonValue::Undefined && value.type() != rule.expectedType) {
            result.valid = false;
            result.errors.append(JsonError::make(JsonError::Code::TypeMismatch,
                                  QStringLiteral("Key '%1' expected type %2, got %3%4")
                                      .arg(rule.key,
                                           QJsonValue(rule.expectedType).toString(),
                                           typeName(value),
                                           rule.description.isEmpty() ? QString() : " (" + rule.description + ")") ));
        }
    }
    return result;
}

BoolResult validateOrThrow(const QJsonObject& object, const Schema& schema)
{
    ValidationResult result = validate(object, schema);
    if (!result.valid) {
        return BoolResult::fail(result.errors.first());
    }
    return BoolResult::ok(true);
}

BoolResult ValidationResult::toResult() const
{
    if (valid) return BoolResult::ok(true);
    return BoolResult::fail(errors.first());
}

// ============================================================================
// Path-based Access
// ============================================================================

namespace {

QStringList splitPath(std::string_view path)
{
    QStringList parts;
    QString current;
    for (char c : path) {
        if (c == '.') {
            if (!current.isEmpty()) parts.append(current);
            current.clear();
        } else if (c == '\\' || c == '/') {
            continue; // Skip path separators within keys
        } else {
            current.append(QChar::fromLatin1(c));
        }
    }
    if (!current.isEmpty()) parts.append(current);
    return parts;
}

QJsonValue valueAt(const QJsonValue& root, const QStringList& parts, int index)
{
    if (index >= parts.size()) return root;

    if (root.isObject()) {
        const QJsonObject& obj = root.toObject();
        if (!obj.contains(parts[index])) return QJsonValue(QJsonValue::Undefined);
        return valueAt(obj.value(parts[index]), parts, index + 1);
    }
    if (root.isArray()) {
        bool ok = false;
        const int arrayIndex = parts[index].toInt(&ok);
        if (!ok) return QJsonValue(QJsonValue::Undefined);
        const QJsonArray& arr = root.toArray();
        if (arrayIndex < 0 || arrayIndex >= arr.size()) return QJsonValue(QJsonValue::Undefined);
        return valueAt(arr[arrayIndex], parts, index + 1);
    }
    return QJsonValue(QJsonValue::Undefined);
}

} // namespace

std::optional<QJsonValue> getValue(const QJsonObject& obj, std::string_view path)
{
    auto result = valueAt(QJsonValue(obj), splitPath(path), 0);
    if (result.isUndefined()) return std::nullopt;
    return result;
}

std::optional<QJsonValue> getValue(const QJsonArray& arr, std::string_view path)
{
    auto result = valueAt(QJsonValue(arr), splitPath(path), 0);
    if (result.isUndefined()) return std::nullopt;
    return result;
}

namespace {

bool setValueImpl(QJsonValue& root, const QStringList& parts, int index, const QJsonValue& value)
{
    if (index >= parts.size()) return false;

    if (root.isObject()) {
        QJsonObject obj = root.toObject();
        if (index == parts.size() - 1) {
            obj[parts[index]] = value;
        } else {
            QJsonValue child = obj.value(parts[index]);
            if (child.isUndefined()) child = QJsonValue(QJsonObject{});
            if (!setValueImpl(child, parts, index + 1, value)) return false;
            obj[parts[index]] = child;
        }
        root = obj;
        return true;
    }

    if (root.isArray()) {
        bool ok = false;
        const int arrayIndex = parts[index].toInt(&ok);
        if (!ok) return false;
        QJsonArray arr = root.toArray();
        while (arr.size() <= arrayIndex) arr.append(QJsonValue(QJsonValue::Undefined));
        if (index == parts.size() - 1) {
            arr[arrayIndex] = value;
        } else {
            QJsonValue child = arr[arrayIndex];
            if (child.isUndefined() || child.isNull()) child = QJsonValue(QJsonObject{});
            if (!setValueImpl(child, parts, index + 1, value)) return false;
            arr[arrayIndex] = child;
        }
        root = arr;
        return true;
    }

    return false;
}

bool removeValueImpl(QJsonValue& root, const QStringList& parts, int index)
{
    if (index >= parts.size()) return false;

    if (root.isObject()) {
        QJsonObject obj = root.toObject();
        if (index == parts.size() - 1) {
            obj.remove(parts[index]);
            return true;
        }
        QJsonValue child = obj.value(parts[index]);
        if (child.isUndefined()) return false;
        if (!removeValueImpl(child, parts, index + 1)) return false;
        obj[parts[index]] = child;
        root = obj;
        return true;
    }

    if (root.isArray()) {
        bool ok = false;
        const int arrayIndex = parts[index].toInt(&ok);
        if (!ok) return false;
        QJsonArray arr = root.toArray();
        if (arrayIndex < 0 || arrayIndex >= arr.size()) return false;
        if (index == parts.size() - 1) {
            arr.removeAt(arrayIndex);
            root = arr;
            return true;
        }
        QJsonValue child = arr[arrayIndex];
        if (child.isUndefined()) return false;
        if (!removeValueImpl(child, parts, index + 1)) return false;
        arr[arrayIndex] = child;
        root = arr;
        return true;
    }

    return false;
}

} // namespace

BoolResult setValue(QJsonObject& obj, std::string_view path, const QJsonValue& value)
{
    QJsonValue root(obj);
    if (!setValueImpl(root, splitPath(path), 0, value)) {
        return BoolResult::fail(JsonError::make(JsonError::Code::TypeMismatch,
                                 QStringLiteral("Cannot set value at path '%1'").arg(QString::fromLatin1(path.data(), static_cast<int>(path.size())))));
    }
    obj = root.toObject();
    return BoolResult::ok(true);
}

BoolResult setValue(QJsonArray& arr, std::string_view path, const QJsonValue& value)
{
    QJsonValue root(arr);
    if (!setValueImpl(root, splitPath(path), 0, value)) {
        return BoolResult::fail(JsonError::make(JsonError::Code::TypeMismatch,
                                 QStringLiteral("Cannot set value at path '%1'").arg(QString::fromLatin1(path.data(), static_cast<int>(path.size())))));
    }
    arr = root.toArray();
    return BoolResult::ok(true);
}

BoolResult removeValue(QJsonObject& obj, std::string_view path)
{
    QJsonValue root(obj);
    if (!removeValueImpl(root, splitPath(path), 0)) {
        return BoolResult::fail(JsonError::make(JsonError::Code::TypeMismatch,
                                 QStringLiteral("Cannot remove value at path '%1'").arg(QString::fromLatin1(path.data(), static_cast<int>(path.size())))));
    }
    obj = root.toObject();
    return BoolResult::ok(true);
}

BoolResult removeValue(QJsonArray& arr, std::string_view path)
{
    QJsonValue root(arr);
    if (!removeValueImpl(root, splitPath(path), 0)) {
        return BoolResult::fail(JsonError::make(JsonError::Code::TypeMismatch,
                                 QStringLiteral("Cannot remove value at path '%1'").arg(QString::fromLatin1(path.data(), static_cast<int>(path.size())))));
    }
    arr = root.toArray();
    return BoolResult::ok(true);
}

// ============================================================================
// Type-safe Extraction
// ============================================================================

template <>
std::optional<QString> extract(const QJsonValue& value)
{
    if (value.isString()) return value.toString();
    if (value.isDouble()) return QString::number(value.toDouble());
    if (value.isBool()) return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    return std::nullopt;
}

template <>
std::optional<int> extract(const QJsonValue& value)
{
    if (value.isDouble()) return static_cast<int>(value.toDouble());
    if (value.isString()) {
        bool ok = false;
        int result = value.toString().toInt(&ok);
        if (ok) return result;
    }
    return std::nullopt;
}

template <>
std::optional<double> extract(const QJsonValue& value)
{
    if (value.isDouble()) return value.toDouble();
    if (value.isString()) {
        bool ok = false;
        double result = value.toString().toDouble(&ok);
        if (ok) return result;
    }
    return std::nullopt;
}

template <>
std::optional<bool> extract(const QJsonValue& value)
{
    if (value.isBool()) return value.toBool();
    if (value.isDouble()) return value.toDouble() != 0.0;
    if (value.isString()) {
        const QString s = value.toString().trimmed().toLower();
        if (s == QStringLiteral("true") || s == QStringLiteral("1") || s == QStringLiteral("yes")) return true;
        if (s == QStringLiteral("false") || s == QStringLiteral("0") || s == QStringLiteral("no")) return false;
    }
    return std::nullopt;
}

template <>
std::optional<QJsonObject> extract(const QJsonValue& value)
{
    if (value.isObject()) return value.toObject();
    return std::nullopt;
}

template <>
std::optional<QJsonArray> extract(const QJsonValue& value)
{
    if (value.isArray()) return value.toArray();
    return std::nullopt;
}

template <>
std::optional<QVariant> extract(const QJsonValue& value)
{
    return value.toVariant();
}

// ============================================================================
// JsonWatcher
// ============================================================================

class JsonWatcher::Private
{
public:
    QFileSystemWatcher watcher;
    std::unordered_map<QString, JsonWatcher::ChangeCallback> callbacks;
};


JsonWatcher::~JsonWatcher() = default;

JsonWatcher::JsonWatcher(QObject* parent)
    : QObject(parent), d(std::make_unique<Private>())
{
    connect(&d->watcher, &QFileSystemWatcher::fileChanged, this,
            [this](const QString& path) {
        // QFileSystemWatcher drops watches on change; re-add
        d->watcher.addPath(path);
        reload(path);
    });
}

bool JsonWatcher::watchFile(const QString& filePath, ChangeCallback callback)
{
    if (d->callbacks.find(filePath) != d->callbacks.end()) {
        return false; // Already watched
    }

    if (!QFileInfo::exists(filePath)) {
        JsonError err{JsonError::Code::FileNotFound,
                      QStringLiteral("Cannot watch missing file: %1").arg(filePath), filePath};
        if (callback) callback(false, QJsonDocument(), err);
        emit fileError(filePath, err);
        return false;
    }

    d->callbacks[filePath] = std::move(callback);
    bool ok = d->watcher.addPath(filePath);

    // Initial load
    reload(filePath);
    return ok;
}

void JsonWatcher::unwatchFile(const QString& filePath)
{
    d->watcher.removePath(filePath);
    d->callbacks.erase(filePath);
}

void JsonWatcher::unwatchAll()
{
    for (const QString& path : d->watcher.files()) {
        d->watcher.removePath(path);
    }
    d->callbacks.clear();
}

void JsonWatcher::reload(const QString& filePath)
{
    auto result = loadFile(filePath);
    if (result.hasError()) {
        if (auto it = d->callbacks.find(filePath); it != d->callbacks.end() && it->second) {
            it->second(false, QJsonDocument(), result.error());
        }
        emit fileError(filePath, result.error());
        return;
    }

    if (auto it = d->callbacks.find(filePath); it != d->callbacks.end() && it->second) {
        it->second(true, result.value(), JsonError{});
    }
    emit fileChanged(filePath, result.value());
}

// ============================================================================
// Migration
// ============================================================================

void JsonMigrator::addMigration(const MigrationStep& step)
{
    m_migrations.append(step);
}

BoolResult JsonMigrator::migrate(QJsonObject& object, int currentVersion, int targetVersion) const
{
    int version = currentVersion;
    while (version < targetVersion) {
        bool found = false;
        for (const auto& step : m_migrations) {
            if (step.fromVersion == version && step.toVersion > version) {
                if (!step.migrate(object)) {
                    return BoolResult::fail(JsonError::make(JsonError::Code::SchemaValidationFailed,
                                             QStringLiteral("Migration failed from version %1 to %2")
                                                 .arg(step.fromVersion).arg(step.toVersion)));
                }
                version = step.toVersion;
                found = true;
                break;
            }
        }
        if (!found) {
            return BoolResult::fail(JsonError::make(JsonError::Code::SchemaValidationFailed,
                                     QStringLiteral("No migration path from version %1 to %2")
                                         .arg(version).arg(targetVersion)));
        }
    }
    return BoolResult::ok(true);
}

// ============================================================================
// Utility Functions
// ============================================================================

QJsonObject merge(const QJsonObject& a, const QJsonObject& b)
{
    QJsonObject result = a;
    for (auto it = b.begin(); it != b.end(); ++it) {
        if (it.value().isObject() && result.contains(it.key()) && result.value(it.key()).isObject()) {
            result[it.key()] = merge(result.value(it.key()).toObject(), it.value().toObject());
        } else {
            result[it.key()] = it.value();
        }
    }
    return result;
}

bool equals(const QJsonValue& a, const QJsonValue& b)
{
    if (a.type() != b.type()) return false;
    switch (a.type()) {
        case QJsonValue::Object: return a.toObject() == b.toObject();
        case QJsonValue::Array: return a.toArray() == b.toArray();
        case QJsonValue::String: return a.toString() == b.toString();
        case QJsonValue::Double: return a.toDouble() == b.toDouble();
        case QJsonValue::Bool: return a.toBool() == b.toBool();
        case QJsonValue::Null: return true;
        case QJsonValue::Undefined: return true;
    }
    return false;
}

QString toString(const QJsonDocument& doc, bool pretty)
{
    return QString::fromUtf8(doc.toJson(pretty ? QJsonDocument::Indented : QJsonDocument::Compact));
}

QString toString(const QJsonObject& obj, bool pretty)
{
    return toString(QJsonDocument(obj), pretty);
}

QString toString(const QJsonArray& arr, bool pretty)
{
    return toString(QJsonDocument(arr), pretty);
}

QString minify(const QString& json)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) return json;
    return toString(doc, false);
}

bool isValidJson(const QString& json)
{
    QJsonParseError error;
    QJsonDocument::fromJson(json.toUtf8(), &error);
    return error.error == QJsonParseError::NoError;
}

QString typeName(const QJsonValue& value)
{
    switch (value.type()) {
        case QJsonValue::Null: return QStringLiteral("null");
        case QJsonValue::Bool: return QStringLiteral("bool");
        case QJsonValue::Double: return QStringLiteral("number");
        case QJsonValue::String: return QStringLiteral("string");
        case QJsonValue::Array: return QStringLiteral("array");
        case QJsonValue::Object: return QStringLiteral("object");
        case QJsonValue::Undefined: return QStringLiteral("undefined");
    }
    return QStringLiteral("unknown");
}

} // namespace arete::json