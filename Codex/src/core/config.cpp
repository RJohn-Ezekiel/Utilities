#include "core/config.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QCryptographicHash>

namespace codex {

Config::Config()
{
    // Sensible defaults
    m_data.mediaInsert = MediaInsert::CopyToVault;
    m_data.autosaveIntervalMs = 3000;
    m_data.fontSize = 13;
    m_data.rememberWindowSize = true;
    m_data.rememberLastNote = true;
}

bool Config::load(const std::filesystem::path &path)
{
    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    auto doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject())
        return false;

    m_data = fromJson(doc.object());
    return true;
}

bool Config::save(const std::filesystem::path &path) const
{
    // Ensure parent dir exists
    std::filesystem::create_directories(path.parent_path());

    QFile file(QString::fromStdString(path.string()));
    if (!file.open(QIODevice::WriteOnly))
        return false;

    auto doc = QJsonDocument(toJson(m_data));
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

QJsonObject Config::toJson(const ConfigData &d)
{
    QJsonObject obj;
    obj["vaultPath"]           = QString::fromStdString(d.vaultPath.string());
    obj["mediaInsert"]         = static_cast<int>(d.mediaInsert);
    obj["autosaveIntervalMs"]  = d.autosaveIntervalMs;
    obj["fontSize"]            = d.fontSize;
    obj["rememberWindowSize"]  = d.rememberWindowSize;
    obj["rememberLastNote"]    = d.rememberLastNote;
    obj["lastOpenedNote"]      = QString::fromStdString(d.lastOpenedNote);
    obj["passwordHash"]        = QString::fromStdString(d.passwordHash);
    obj["vaultLocked"]         = d.vaultLocked;
    return obj;
}

ConfigData Config::fromJson(const QJsonObject &obj)
{
    ConfigData d;
    if (obj.contains("vaultPath"))
        d.vaultPath = obj["vaultPath"].toString().toStdString();
    if (obj.contains("mediaInsert"))
        d.mediaInsert = static_cast<MediaInsert>(obj["mediaInsert"].toInt());
    if (obj.contains("autosaveIntervalMs"))
        d.autosaveIntervalMs = obj["autosaveIntervalMs"].toInt();
    if (obj.contains("fontSize"))
        d.fontSize = obj["fontSize"].toInt();
    if (obj.contains("rememberWindowSize"))
        d.rememberWindowSize = obj["rememberWindowSize"].toBool();
    if (obj.contains("rememberLastNote"))
        d.rememberLastNote = obj["rememberLastNote"].toBool();
    if (obj.contains("lastOpenedNote"))
        d.lastOpenedNote = obj["lastOpenedNote"].toString().toStdString();
    if (obj.contains("passwordHash"))
        d.passwordHash = obj["passwordHash"].toString().toStdString();
    if (obj.contains("vaultLocked"))
        d.vaultLocked = obj["vaultLocked"].toBool();
    return d;
}

std::string Config::hashPassword(const QString &password)
{
    auto hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex()).toStdString();
}

} // namespace codex
