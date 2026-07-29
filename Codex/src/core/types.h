#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace codex {

// ── Identifiers ─────────────────────────────────────────────────

using NoteId = std::uint64_t;

// ── Media insert behaviour ──────────────────────────────────────

enum class MediaInsert {
    Relative,
    Absolute,
    CopyToVault
};

// ── Configuration ───────────────────────────────────────────────

struct ConfigData {
    std::filesystem::path vaultPath;
    MediaInsert mediaInsert = MediaInsert::CopyToVault;
    int autosaveIntervalMs = 3000;
    int fontSize = 13;
    bool rememberWindowSize = true;
    bool rememberLastNote = true;
    std::string lastOpenedNote;
    std::string passwordHash;
    bool vaultLocked = false;
};

// ── Note ────────────────────────────────────────────────────────

struct Note {
    std::filesystem::path path;       // relative to vault root
    std::string title;
    std::string content;
    std::vector<std::string> tags;
    std::vector<std::string> wikiLinks;
    bool isJournal  = false;
    bool isTemplate = false;
    bool isDeleted  = false;          // sits in Trash/

    [[nodiscard]] std::filesystem::path fullPath(const std::filesystem::path &vaultRoot) const {
        return vaultRoot / path;
    }

    [[nodiscard]] bool isValid() const noexcept {
        return !path.empty();
    }
};

// ── Search result ───────────────────────────────────────────────

struct SearchResult {
    std::filesystem::path notePath;
    std::string title;
    std::string snippet;      // context around match
    float relevance = 0.0f;
};

// ── Vault structure constants ───────────────────────────────────

namespace vault_layout {

inline const std::string DIR_NOTES     = "Notes";
inline const std::string DIR_MEDIA     = "Media";
inline const std::string DIR_IMAGES    = "Media/Images";
inline const std::string DIR_VIDEOS    = "Media/Videos";
inline const std::string DIR_AUDIO     = "Media/Audio";
inline const std::string DIR_FILES     = "Media/Files";
inline const std::string DIR_JOURNAL   = "Journal";
inline const std::string DIR_TEMPLATES = "Templates";
inline const std::string DIR_EXPORTS   = "Exports";
inline const std::string DIR_TRASH     = "Trash";
inline const std::string DIR_CONFIG    = "Config";
inline const std::string FILE_CONFIG   = "Config/config.json";
inline const std::string FILE_BACKLINKS = "Config/backlinks.json";
inline const std::string FILE_TAGS     = "Config/tags.json";
inline const std::string FILE_README   = "README.md";

} // namespace vault_layout

} // namespace codex
