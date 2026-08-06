#pragma once

#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonParseError>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace arete::json {

// Error type for JSON operations
struct JsonError
{
    enum class Code {
        None,
        FileNotFound,
        PermissionDenied,
        InvalidJson,
        TypeMismatch,
        MissingKey,
        SchemaValidationFailed,
        WriteFailed
    } code = Code::None;

    QString message;
    QString filePath;
    int line = 0;
    int column = 0;

    [[nodiscard]] bool hasError() const { return code != Code::None; }
    [[nodiscard]] QString toString() const;

    // Convenience factory avoiding partial aggregate initialization.
    [[nodiscard]] static JsonError make(Code code, QString message)
    {
        return {code, std::move(message), {}, 0, 0};
    }

    [[nodiscard]] static JsonError make(Code code, QString message, QString filePath)
    {
        return {code, std::move(message), std::move(filePath), 0, 0};
    }

    [[nodiscard]] static JsonError make(Code code, QString message, QString filePath,
                                        int line, int column)
    {
        return {code, std::move(message), std::move(filePath), line, column};
    }
};

// Result type combining a value with an error
template <typename T>
class Result
{
public:
    Result() = default;

    static Result ok(T value) { Result r; r.m_value = std::move(value); return r; }
    static Result fail(JsonError error) { Result r; r.m_error = std::move(error); return r; }

    [[nodiscard]] bool hasError() const { return m_error.hasError(); }
    [[nodiscard]] const JsonError& error() const { return m_error; }
    [[nodiscard]] const T& value() const { return m_value; }
    [[nodiscard]] T& value() { return m_value; }
    [[nodiscard]] const T& operator*() const { return m_value; }
    [[nodiscard]] const T* operator->() const { return &m_value; }

private:
    T m_value{};
    JsonError m_error{};
};

using JsonResult = Result<QJsonDocument>;
using JsonObjectResult = Result<QJsonObject>;
using JsonArrayResult = Result<QJsonArray>;
using BoolResult = Result<bool>;

// ============================================================================
// File Operations
// ============================================================================

// Load JSON from file with detailed error reporting
[[nodiscard]] JsonResult loadFile(const QString& filePath);

// Load JSON object from file
[[nodiscard]] JsonObjectResult loadObject(const QString& filePath);

// Load JSON array from file
[[nodiscard]] JsonArrayResult loadArray(const QString& filePath);

// Save JSON to file (atomic write with backup)
[[nodiscard]] BoolResult saveFile(const QString& filePath, const QJsonDocument& doc,
                                  bool createBackup = true, bool prettyPrint = true);

// Save JSON object to file
[[nodiscard]] BoolResult saveObject(const QString& filePath, const QJsonObject& obj,
                                    bool createBackup = true, bool prettyPrint = true);

// Save JSON array to file
[[nodiscard]] BoolResult saveArray(const QString& filePath, const QJsonArray& arr,
                                   bool createBackup = true, bool prettyPrint = true);

// ============================================================================
// Validation
// ============================================================================

struct ValidationRule
{
    QString key;
    QJsonValue::Type expectedType = QJsonValue::Undefined;
    bool required = false;
    std::function<bool(const QJsonValue&)> customValidator;
    QString description;
};

struct Schema
{
    QVector<ValidationRule> rules;

    Schema& required(const QString& key, QJsonValue::Type type,
                     const QString& description = QString());
    Schema& optional(const QString& key, QJsonValue::Type type,
                     const QString& description = QString());
    Schema& custom(const QString& key, std::function<bool(const QJsonValue&)> validator,
                   const QString& description = QString());
};

struct ValidationResult
{
    bool valid = true;
    QVector<JsonError> errors;

    [[nodiscard]] BoolResult toResult() const;
};

[[nodiscard]] ValidationResult validate(const QJsonObject& object, const Schema& schema);
[[nodiscard]] BoolResult validateOrThrow(const QJsonObject& object, const Schema& schema);

// ============================================================================
// Path-based Access (dot notation)
// ============================================================================

// Get value at path (e.g., "user.address.city")
[[nodiscard]] std::optional<QJsonValue> getValue(const QJsonObject& obj, std::string_view path);
[[nodiscard]] std::optional<QJsonValue> getValue(const QJsonArray& arr, std::string_view path);

// Set value at path (creates intermediate objects/arrays)
[[nodiscard]] BoolResult setValue(QJsonObject& obj, std::string_view path, const QJsonValue& value);
[[nodiscard]] BoolResult setValue(QJsonArray& arr, std::string_view path, const QJsonValue& value);

// Remove value at path
[[nodiscard]] BoolResult removeValue(QJsonObject& obj, std::string_view path);
[[nodiscard]] BoolResult removeValue(QJsonArray& arr, std::string_view path);

// ============================================================================
// Type-safe Value Extraction
// ============================================================================

template <typename T>
[[nodiscard]] std::optional<T> extract(const QJsonValue& value);

template <>
[[nodiscard]] std::optional<QString> extract(const QJsonValue& value);

template <>
[[nodiscard]] std::optional<int> extract(const QJsonValue& value);

template <>
[[nodiscard]] std::optional<double> extract(const QJsonValue& value);

template <>
[[nodiscard]] std::optional<bool> extract(const QJsonValue& value);

template <>
[[nodiscard]] std::optional<QJsonObject> extract(const QJsonValue& value);

template <>
[[nodiscard]] std::optional<QJsonArray> extract(const QJsonValue& value);

template <>
[[nodiscard]] std::optional<QVariant> extract(const QJsonValue& value);

// Extract with default
template <typename T>
[[nodiscard]] T extractOr(const QJsonValue& value, const T& defaultValue)
{
    auto result = extract<T>(value);
    return result.has_value() ? *result : defaultValue;
}

// Extract from object at path
template <typename T>
[[nodiscard]] std::optional<T> extractAt(const QJsonObject& obj, std::string_view path)
{
    auto value = getValue(obj, path);
    if (!value.has_value()) return std::nullopt;
    return extract<T>(*value);
}

template <typename T>
[[nodiscard]] T extractAtOr(const QJsonObject& obj, std::string_view path, const T& defaultValue)
{
    auto value = extractAt<T>(obj, path);
    return value.has_value() ? *value : defaultValue;
}

// ============================================================================
// JSON Watcher (file change detection)
// ============================================================================

class JsonWatcher : public QObject
{
    Q_OBJECT

public:
    explicit JsonWatcher(QObject* parent = nullptr);
    ~JsonWatcher() override;

    // Watch a JSON file for changes
    // callback receives (success, document, error)
    using ChangeCallback = std::function<void(bool, const QJsonDocument&, const JsonError&)>;

    bool watchFile(const QString& filePath, ChangeCallback callback);
    void unwatchFile(const QString& filePath);
    void unwatchAll();

    // Force reload
    void reload(const QString& filePath);

signals:
    void fileChanged(const QString& filePath, const QJsonDocument& document);
    void fileError(const QString& filePath, const JsonError& error);

private:
    class Private;
    std::unique_ptr<Private> d;
};

// ============================================================================
// Migration / Versioning
// ============================================================================

struct MigrationStep
{
    int fromVersion;
    int toVersion;
    std::function<bool(QJsonObject&)> migrate;
};

class JsonMigrator
{
public:
    JsonMigrator() = default;
    ~JsonMigrator() = default;

    void addMigration(const MigrationStep& step);
    [[nodiscard]] BoolResult migrate(QJsonObject& object, int currentVersion, int targetVersion) const;

private:
    QVector<MigrationStep> m_migrations;
};

// ============================================================================
// Utility Functions
// ============================================================================

// Deep merge two objects (b overrides a)
[[nodiscard]] QJsonObject merge(const QJsonObject& a, const QJsonObject& b);

// Compare two JSON values for equality (ignoring key order)
[[nodiscard]] bool equals(const QJsonValue& a, const QJsonValue& b);

// Pretty print to string
[[nodiscard]] QString toString(const QJsonDocument& doc, bool pretty = true);
[[nodiscard]] QString toString(const QJsonObject& obj, bool pretty = true);
[[nodiscard]] QString toString(const QJsonArray& arr, bool pretty = true);

// Minify (remove whitespace)
[[nodiscard]] QString minify(const QString& json);

// Check if string is valid JSON
[[nodiscard]] bool isValidJson(const QString& json);

// Get JSON value type as string
[[nodiscard]] QString typeName(const QJsonValue& value);

} // namespace arete::json