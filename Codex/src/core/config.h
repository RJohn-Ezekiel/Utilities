#pragma once

#include "core/types.h"

#include <QJsonObject>
#include <QString>

namespace codex {

class Config {
public:
    Config();

    // Load from / save to a JSON file
    bool load(const std::filesystem::path &path);
    bool save(const std::filesystem::path &path) const;

    // Getters / setters
    const ConfigData &data() const noexcept { return m_data; }
    void setData(const ConfigData &d) { m_data = d; }

    std::filesystem::path vaultPath() const noexcept { return m_data.vaultPath; }
    void setVaultPath(const std::filesystem::path &p) { m_data.vaultPath = p; }

    MediaInsert mediaInsert() const noexcept { return m_data.mediaInsert; }
    void setMediaInsert(MediaInsert m) { m_data.mediaInsert = m; }

    int autosaveIntervalMs() const noexcept { return m_data.autosaveIntervalMs; }
    void setAutosaveIntervalMs(int ms) { m_data.autosaveIntervalMs = ms; }

    int fontSize() const noexcept { return m_data.fontSize; }
    void setFontSize(int s) { m_data.fontSize = s; }

    bool rememberWindowSize() const noexcept { return m_data.rememberWindowSize; }
    void setRememberWindowSize(bool b) { m_data.rememberWindowSize = b; }

    bool rememberLastNote() const noexcept { return m_data.rememberLastNote; }
    void setRememberLastNote(bool b) { m_data.rememberLastNote = b; }

    std::string lastOpenedNote() const noexcept { return m_data.lastOpenedNote; }
    void setLastOpenedNote(const std::string &s) { m_data.lastOpenedNote = s; }

    std::string passwordHash() const noexcept { return m_data.passwordHash; }
    void setPasswordHash(const std::string &h) { m_data.passwordHash = h; }

    bool vaultLocked() const noexcept { return m_data.vaultLocked; }
    void setVaultLocked(bool locked) { m_data.vaultLocked = locked; }

    bool hasPassword() const noexcept { return !m_data.passwordHash.empty(); }

    static std::string hashPassword(const QString &password);

private:
    ConfigData m_data;

    static QJsonObject toJson(const ConfigData &d);
    static ConfigData fromJson(const QJsonObject &obj);
};

} // namespace codex
