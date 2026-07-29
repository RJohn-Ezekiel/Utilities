#include "cli/cli.h"
#include "export/export.h"

#include <iostream>
#include <algorithm>

namespace codex {

CLI::CLI(const std::filesystem::path &vaultPath) : m_vaultPath(vaultPath) {
    m_vault = new VaultManager(nullptr);
    if (!m_vault->openVault(vaultPath)) {
        qCritical().noquote() << "Failed to open vault:" << QString::fromStdString(vaultPath.string());
        delete m_vault;
        m_vault = nullptr;
    }
}

CLI::~CLI() {
    delete m_vault;
}

int CLI::run(const QCommandLineParser &parser) {
    if (!m_vault) {
        qCritical().noquote() << "Vault is not open.";
        return 1;
    }

    if (parser.isSet(QStringLiteral("list")))
        return listNotes();

    if (parser.isSet(QStringLiteral("show")))
        return showNote(parser.value(QStringLiteral("show")));

    if (parser.isSet(QStringLiteral("new")))
        return createNote(parser.value(QStringLiteral("new")));

    if (parser.isSet(QStringLiteral("search")))
        return searchNotes(parser.value(QStringLiteral("search")));

    if (parser.isSet(QStringLiteral("export")) || parser.isSet(QStringLiteral("all"))) {
        QString outputDir = parser.value(QStringLiteral("output"));
        if (outputDir.isEmpty())
            outputDir = QString::fromStdString((m_vaultPath / vault_layout::DIR_EXPORTS).string());

        if (parser.isSet(QStringLiteral("all")))
            return exportAll(outputDir);

        if (parser.isSet(QStringLiteral("export")))
            return exportNote(parser.value(QStringLiteral("export")), outputDir);
    }

    return 0;
}

int CLI::listNotes() {
    auto notes = m_vault->listNotes();

    qInfo().noquote() << "Title | Path | Tags";
    qInfo().noquote() << "----- | ---- | ----";
    for (const auto &note : notes) {
        QStringList tagList;
        for (const auto &t : note.tags)
            tagList << QString::fromStdString(t);

        qInfo().noquote()
            << QString::fromStdString(note.title)
            << " | "
            << QString::fromStdString(note.path.string())
            << " | "
            << tagList.join(QStringLiteral(", "));
    }
    return 0;
}

int CLI::showNote(const QString &title) {
    std::optional<Note> note;

    auto tryPath = std::filesystem::path(title.toStdString());
    note = m_vault->openNote(tryPath);

    if (!note) {
        auto relPath = m_vault->resolveWikiLink(title.toStdString());
        if (relPath)
            note = m_vault->openNote(*relPath);
    }

    if (!note) {
        qInfo().noquote() << "Note not found:" << title;
        return 1;
    }

    qInfo().noquote() << QString::fromStdString(note->title);
    qInfo().noquote() << QString::fromStdString(note->path.string());
    qInfo().noquote() << "---";
    qInfo().noquote() << QString::fromStdString(note->content);
    return 0;
}

int CLI::createNote(const QString &title) {
    if (title.isEmpty()) {
        qInfo().noquote() << "Title cannot be empty.";
        return 1;
    }

    if (m_vault->createNote(title.toStdString())) {
        qInfo().noquote() << "Note created:" << title;
        return 0;
    }

    qInfo().noquote() << "Failed to create note:" << title;
    return 1;
}

int CLI::searchNotes(const QString &query) {
    auto notes = m_vault->listNotes();
    auto qLower = query.toLower();
    int found = 0;

    for (const auto &note : notes) {
        auto titleLower = QString::fromStdString(note.title).toLower();
        auto contentLower = QString::fromStdString(note.content).toLower();

        if (titleLower.contains(qLower) || contentLower.contains(qLower)) {
            qInfo().noquote() << QString::fromStdString(note.title);
            qInfo().noquote() << "  " << QString::fromStdString(note.path.string());
            ++found;
        }
    }

    if (found == 0)
        qInfo().noquote() << "No notes found for:" << query;

    return 0;
}

int CLI::exportNote(const QString &target, const QString &outputDir) {
    std::optional<std::filesystem::path> relPath;

    auto tryPath = std::filesystem::path(target.toStdString());
    auto note = m_vault->openNote(tryPath);
    if (note) {
        relPath = tryPath;
    } else {
        relPath = m_vault->resolveWikiLink(target.toStdString());
        if (relPath)
            note = m_vault->openNote(*relPath);
    }

    if (!relPath || !note) {
        qInfo().noquote() << "Note not found:" << target;
        return 1;
    }
    if (!note) {
        qInfo().noquote() << "Failed to open note:" << target;
        return 1;
    }

    ExportManager exportMgr(m_vaultPath);
    if (exportMgr.exportNote(*note, outputDir.toStdString())) {
        qInfo().noquote() << "Exported:" << target;
        return 0;
    }

    qInfo().noquote() << "Failed to export:" << target;
    return 1;
}

int CLI::exportAll(const QString &outputDir) {
    auto notes = m_vault->listNotes();
    std::vector<Note> fullNotes;
    for (const auto &n : notes) {
        auto full = m_vault->openNote(n.path);
        if (full)
            fullNotes.push_back(std::move(*full));
    }

    if (!fullNotes.empty()) {
        ExportManager exportMgr(m_vaultPath);
        if (exportMgr.exportAll(fullNotes, outputDir.toStdString())) {
            qInfo().noquote() << "Exported all notes to:" << outputDir;
            return 0;
        }
    }

    qInfo().noquote() << "Export failed.";
    return 1;
}

} // namespace codex
