#pragma once

#include "core/vault.h"
#include "core/config.h"
#include <QCommandLineParser>
#include <filesystem>

namespace codex {

class CLI {
public:
    explicit CLI(const std::filesystem::path &vaultPath);
    ~CLI();

    int run(const QCommandLineParser &parser);

private:
    std::filesystem::path m_vaultPath;
    VaultManager *m_vault;

    int listNotes();
    int showNote(const QString &title);
    int createNote(const QString &title);
    int searchNotes(const QString &query);
    int exportNote(const QString &target, const QString &outputDir);
    int exportAll(const QString &outputDir);
};

} // namespace codex
