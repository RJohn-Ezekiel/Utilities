#pragma once

#include "core/types.h"

#include <QObject>
#include <QTimer>

#include <vector>
#include <optional>
#include <functional>

namespace codex {

class VaultManager : public QObject {
    Q_OBJECT

public:
    explicit VaultManager(QObject *parent = nullptr);

    // Vault lifecycle
    bool createVault(const std::filesystem::path &root);
    bool openVault(const std::filesystem::path &root);
    bool ensureVault(const std::filesystem::path &root);
    void closeVault();
    [[nodiscard]] bool isOpen() const noexcept { return m_open; }
    [[nodiscard]] const std::filesystem::path &vaultRoot() const noexcept { return m_root; }

    // Note CRUD — paths are relative to vault root
    std::vector<Note> listNotes() const;
    std::optional<Note> openNote(const std::filesystem::path &relativePath) const;
    bool saveNote(const Note &note);
    bool createNote(const std::string &title, const std::filesystem::path &folder = "Notes");
    bool renameNote(const std::filesystem::path &oldRelative, const std::string &newTitle);
    bool duplicateNote(const std::filesystem::path &relativePath);
    bool deleteNote(const std::filesystem::path &relativePath);    // → Trash
    bool restoreNote(const std::filesystem::path &trashRelative);  // ← Trash
    bool permanentlyDelete(const std::filesystem::path &trashRelative);
    bool moveNote(const std::filesystem::path &relativePath,
                  const std::filesystem::path &newFolder);

    // Folder management
    bool createFolder(const std::filesystem::path &folderPath);
    bool isEmptyFolder(const std::filesystem::path &folderPath) const;

    // Wiki link resolution
    std::optional<std::filesystem::path> resolveWikiLink(const std::string &title) const;

    // Utility
    [[nodiscard]] std::filesystem::path absPath(const std::filesystem::path &relative) const;
    [[nodiscard]] std::filesystem::path relPath(const std::filesystem::path &absolute) const;
    [[nodiscard]] std::string noteTitleFromPath(const std::filesystem::path &relative) const;
    [[nodiscard]] std::string extractTitleFromContent(const std::string &content) const;

signals:
    void vaultOpened(const std::filesystem::path &root);
    void vaultClosed();
    void noteCreated(const std::filesystem::path &relativePath);
    void noteDeleted(const std::filesystem::path &relativePath);
    void noteRestored(const std::filesystem::path &relativePath);
    void noteRenamed(const std::filesystem::path &oldPath, const std::filesystem::path &newPath);
    void noteSaved(const std::filesystem::path &relativePath);

private:
    bool m_open = false;
    std::filesystem::path m_root;

    static bool isMarkdownFile(const std::filesystem::path &p);
    static std::vector<std::string> extractTags(const std::string &content);
    static std::vector<std::string> extractWikiLinks(const std::string &content);
    void ensureDir(const std::filesystem::path &dir);
    bool initVaultStructure(const std::filesystem::path &root);
};

} // namespace codex
