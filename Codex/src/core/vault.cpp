#include "core/vault.h"
#include "core/util.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <ranges>

namespace codex {

VaultManager::VaultManager(QObject *parent)
    : QObject(parent)
{
}

// ── Vault lifecycle ─────────────────────────────────────────────

bool VaultManager::createVault(const std::filesystem::path &root)
{
    if (m_open) closeVault();
    if (!initVaultStructure(root)) return false;
    m_root = root;
    m_open = true;
    emit vaultOpened(root);
    return true;
}

bool VaultManager::openVault(const std::filesystem::path &root)
{
    if (m_open) closeVault();

    // Verify this looks like a Codex vault
    if (!exists(root / vault_layout::DIR_NOTES))
        return false;

    m_root = root;
    m_open = true;
    emit vaultOpened(root);
    return true;
}

bool VaultManager::ensureVault(const std::filesystem::path &root)
{
    if (m_open) closeVault();

    // If already a vault, ensure all directories exist then open it
    if (exists(root / vault_layout::DIR_NOTES)) {
        initVaultStructure(root);
        m_root = root;
        m_open = true;
        emit vaultOpened(root);
        return true;
    }

    // Auto-create vault structure
    return createVault(root);
}

bool VaultManager::initVaultStructure(const std::filesystem::path &root)
{
    try {
        ensureDir(root / vault_layout::DIR_NOTES);
        ensureDir(root / vault_layout::DIR_IMAGES);
        ensureDir(root / vault_layout::DIR_VIDEOS);
        ensureDir(root / vault_layout::DIR_AUDIO);
        ensureDir(root / vault_layout::DIR_FILES);
        ensureDir(root / vault_layout::DIR_JOURNAL);
        ensureDir(root / vault_layout::DIR_TEMPLATES);
        ensureDir(root / vault_layout::DIR_EXPORTS);
        ensureDir(root / vault_layout::DIR_TRASH);
        ensureDir(root / vault_layout::DIR_CONFIG);

        // Write README
        auto readmePath = root / "README.md";
        if (!exists(readmePath)) {
            std::ofstream readme(readmePath);
            if (readme) {
                readme << "# Codex Vault\n\n"
                       << "Your notes are stored as plain Markdown files.\n"
                       << "Open this folder with any text editor.\n";
            }
        }

        // Write default Welcome note
        auto welcomePath = root / vault_layout::DIR_NOTES / "Welcome.md";
        if (!exists(welcomePath)) {
            std::ofstream f(welcomePath);
            if (f) {
                f << "# Welcome to Codex\n\n"
                  << "This is your first note. Start writing!\n\n"
                  << "## Features\n\n"
                  << "- **Markdown**: headings, bold, italic, lists, checkboxes, code blocks\n"
                  << "- **Wiki links**: [[Link to another note]]\n"
                  << "- **Tags**: #project #feature\n"
                  << "- **Search**: full-text search across all notes\n"
                  << "- **Export**: HTML, Markdown, or Plain Text\n\n"
                  << "## Getting Started\n\n"
                  << "Create a new note with `Ctrl+N` or the toolbar button.\n"
                  << "Toggle between editing and reading view with `F5`.\n";
            }
        }

        // Write default Journal template
        auto journalPath = root / vault_layout::DIR_TEMPLATES / "Journal.md";
        if (!exists(journalPath)) {
            std::ofstream f(journalPath);
            if (f) {
                f << "# {{date}}\n\n"
                  << "## Tasks\n\n"
                  << "- [ ] \n\n"
                  << "## Notes\n\n\n"
                  << "## Reflection\n\n";
            }
        }

        // Create empty config files if they don't exist
        for (const auto &cfgFile : {vault_layout::FILE_CONFIG,
                                     vault_layout::FILE_BACKLINKS,
                                     vault_layout::FILE_TAGS}) {
            auto p = root / cfgFile;
            if (!exists(p)) {
                std::ofstream f(p);
                if (f) f << "{}\n";
            }
        }

        return true;
    } catch (...) {
        return false;
    }
}

void VaultManager::closeVault()
{
    if (!m_open) return;
    m_open = false;
    m_root.clear();
    emit vaultClosed();
}

// ── Note CRUD ───────────────────────────────────────────────────

std::vector<Note> VaultManager::listNotes() const
{
    if (!m_open) return {};

    std::vector<Note> notes;
    auto scanDir = [&](const std::filesystem::path &dir, bool isJournal, bool isTemplate) -> void {
        if (!exists(dir)) return;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            if (!isMarkdownFile(entry.path())) continue;

            Note note;
            note.path = relPath(entry.path());
            note.isJournal = isJournal;
            note.isTemplate = isTemplate;

            // Read first line for title
            std::ifstream f(entry.path());
            std::string line;
            while (std::getline(f, line)) {
                if (line.starts_with("# ")) {
                    note.title = line.substr(2);
                    break;
                }
                if (!line.empty() && !line.starts_with("---"))
                    break;  // No heading found in first paragraph
            }

            if (note.title.empty())
                note.title = note.path.stem().string();

            notes.push_back(std::move(note));
        }
    };

    scanDir(m_root / vault_layout::DIR_NOTES, false, false);
    scanDir(m_root / vault_layout::DIR_JOURNAL, true, false);
    scanDir(m_root / vault_layout::DIR_TEMPLATES, false, true);
    scanDir(m_root / vault_layout::DIR_TRASH, false, false);

    return notes;
}

std::optional<Note> VaultManager::openNote(const std::filesystem::path &relativePath) const
{
    if (!m_open) return std::nullopt;

    auto full = absPath(relativePath);
    if (!exists(full))
        return std::nullopt;

    Note note;
    note.path = relativePath;

    // Read entire content
    std::ifstream f(full);
    if (!f) return std::nullopt;

    std::ostringstream buf;
    buf << f.rdbuf();
    note.content = buf.str();

    // Derive metadata
    note.title = extractTitleFromContent(note.content);
    if (note.title.empty())
        note.title = relativePath.stem().string();

    note.tags = extractTags(note.content);
    note.wikiLinks = extractWikiLinks(note.content);

    // Detect folder
    auto parent = relativePath.parent_path();
    note.isJournal  = (parent == vault_layout::DIR_JOURNAL);
    note.isTemplate = (parent == vault_layout::DIR_TEMPLATES);
    note.isDeleted  = (parent == vault_layout::DIR_TRASH);

    return note;
}

bool VaultManager::saveNote(const Note &note)
{
    if (!m_open) return false;

    auto full = absPath(note.path);
    std::ofstream f(full);
    if (!f) return false;

    f << note.content;
    f.close();

    emit noteSaved(note.path);
    return true;
}

bool VaultManager::createNote(const std::string &title, const std::filesystem::path &folder)
{
    if (!m_open) return false;

    auto filename = util::sanitizeFilename(title);
    auto notePath = folder / (filename + ".md");
    auto full = absPath(notePath);

    // Ensure unique
    if (exists(full)) {
        int counter = 1;
        while (exists(full)) {
            notePath = folder / (filename + "_" + std::to_string(counter) + ".md");
            full = absPath(notePath);
            counter++;
        }
    }

    std::ofstream f(full);
    if (!f) return false;

    f << "# " << title << "\n\n";
    f.close();

    emit noteCreated(notePath);
    return true;
}

bool VaultManager::renameNote(const std::filesystem::path &oldRelative, const std::string &newTitle)
{
    if (!m_open) return false;

    auto oldFull = absPath(oldRelative);
    if (!exists(oldFull)) return false;

    auto newName = util::sanitizeFilename(newTitle);
    auto newRelative = oldRelative.parent_path() / (newName + ".md");
    auto newFull = absPath(newRelative);

    if (exists(newFull)) return false;

    std::filesystem::rename(oldFull, newFull);
    emit noteRenamed(oldRelative, newRelative);
    return true;
}

bool VaultManager::duplicateNote(const std::filesystem::path &relativePath)
{
    if (!m_open) return false;

    auto full = absPath(relativePath);
    if (!exists(full)) return false;

    auto stem = relativePath.stem().string();
    auto ext = relativePath.extension().string();
    auto parent = relativePath.parent_path();

    auto newPath = parent / (stem + "_copy" + ext);
    auto newFull = absPath(newPath);

    int counter = 1;
    while (exists(newFull)) {
        newPath = parent / (stem + "_copy_" + std::to_string(counter) + ext);
        newFull = absPath(newPath);
        counter++;
    }

    std::filesystem::copy_file(full, newFull);
    emit noteCreated(newPath);
    return true;
}

bool VaultManager::deleteNote(const std::filesystem::path &relativePath)
{
    if (!m_open) return false;

    auto full = absPath(relativePath);
    if (!exists(full)) return false;

    auto trashPath = absPath(vault_layout::DIR_TRASH) / relativePath.filename();
    auto uniqueTrash = trashPath;

    int counter = 1;
    while (exists(uniqueTrash)) {
        auto stem = trashPath.stem().string();
        uniqueTrash = trashPath.parent_path() / (stem + "_" + std::to_string(counter) + ".md");
        counter++;
    }

    std::filesystem::rename(full, uniqueTrash);
    emit noteDeleted(relativePath);
    return true;
}

bool VaultManager::restoreNote(const std::filesystem::path &trashRelative)
{
    if (!m_open) return false;

    auto full = absPath(trashRelative);
    if (!exists(full)) return false;

    auto restoredPath = absPath(vault_layout::DIR_NOTES) / trashRelative.filename();
    if (exists(restoredPath)) return false;

    std::filesystem::rename(full, restoredPath);

    auto restoredRelative = std::filesystem::path(vault_layout::DIR_NOTES) / trashRelative.filename();
    emit noteRestored(restoredRelative);
    return true;
}

bool VaultManager::permanentlyDelete(const std::filesystem::path &trashRelative)
{
    if (!m_open) return false;
    return std::filesystem::remove(absPath(trashRelative));
}

bool VaultManager::moveNote(const std::filesystem::path &relativePath,
                            const std::filesystem::path &newFolder)
{
    if (!m_open) return false;

    auto full = absPath(relativePath);
    if (!exists(full)) return false;

    auto newRelative = newFolder / relativePath.filename();
    auto newFull = absPath(newRelative);

    if (exists(newFull)) return false;

    create_directories(absPath(newFolder));
    std::filesystem::rename(full, newFull);
    emit noteRenamed(relativePath, newRelative);
    return true;
}

// ── Folder management ───────────────────────────────────────────

bool VaultManager::createFolder(const std::filesystem::path &folderPath)
{
    if (!m_open) return false;
    auto full = absPath(folderPath);
    return create_directories(full);
}

bool VaultManager::isEmptyFolder(const std::filesystem::path &folderPath) const
{
    if (!m_open) return false;
    auto full = absPath(folderPath);
    if (!exists(full)) return true;
    for (const auto &entry : std::filesystem::directory_iterator(full)) {
        if (isMarkdownFile(entry.path()))
            return false;
    }
    return true;
}

// ── Wiki link resolution ────────────────────────────────────────

std::optional<std::filesystem::path> VaultManager::resolveWikiLink(const std::string &title) const
{
    if (!m_open) return std::nullopt;

    // Search all notes folders for a matching title
    auto searchDir = [&](const std::filesystem::path &dir) -> std::optional<std::filesystem::path> {
        if (!exists(dir)) return std::nullopt;

        for (const auto &entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file() || !isMarkdownFile(entry.path()))
                continue;

            auto stem = entry.path().stem().string();
            if (stem == title)
                return relPath(entry.path());
        }

        // Case-insensitive fallback
        auto lower = [](std::string s) { std::ranges::transform(s, s.begin(), ::tolower); return s; };
        auto target = lower(title);

        for (const auto &entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file() || !isMarkdownFile(entry.path()))
                continue;

            auto stem = lower(entry.path().stem().string());
            if (stem == target)
                return relPath(entry.path());
        }

        return std::nullopt;
    };

    auto result = searchDir(m_root / vault_layout::DIR_NOTES);
    if (result) return result;

    result = searchDir(m_root / vault_layout::DIR_JOURNAL);
    if (result) return result;

    return std::nullopt;
}

// ── Utilities ───────────────────────────────────────────────────

std::filesystem::path VaultManager::absPath(const std::filesystem::path &relative) const
{
    return m_root / relative;
}

std::filesystem::path VaultManager::relPath(const std::filesystem::path &absolute) const
{
    return std::filesystem::relative(absolute, m_root);
}

std::string VaultManager::noteTitleFromPath(const std::filesystem::path &relative) const
{
    return relative.stem().string();
}

std::string VaultManager::extractTitleFromContent(const std::string &content) const
{
    // Look for first # Heading
    for (const auto &line : std::views::split(content, '\n')) {
        std::string_view sv(line.begin(), line.end());
        if (sv.starts_with("# ")) {
            return std::string(sv.substr(2));
        }
        // Stop at first non-empty, non-frontmatter line
        if (!sv.empty() && !sv.starts_with("---"))
            break;
    }
    return {};
}

bool VaultManager::isMarkdownFile(const std::filesystem::path &p)
{
    auto ext = p.extension().string();
    return ext == ".md" || ext == ".markdown";
}

std::vector<std::string> VaultManager::extractTags(const std::string &content)
{
    std::vector<std::string> tags;
    bool inCodeBlock = false;

    static const QRegularExpression tagRe(R"((?<!\w)#(\w[\w-]*))");

    for (const auto &line : std::views::split(content, '\n')) {
        std::string_view sv(line.begin(), line.end());

        // Track code blocks
        if (sv.starts_with("```"))
            inCodeBlock = !inCodeBlock;
        if (inCodeBlock)
            continue;

        // Skip headings, code spans, URLs — simplest approach: find all #tags
        auto qline = QString::fromUtf8(sv.data(), static_cast<int>(sv.size()));
        auto it = tagRe.globalMatch(qline);
        while (it.hasNext()) {
            auto m = it.next();
            auto tag = m.captured(1).toStdString();
            if (!tag.empty() && std::ranges::find(tags, tag) == tags.end())
                tags.push_back(std::move(tag));
        }
    }

    return tags;
}

std::vector<std::string> VaultManager::extractWikiLinks(const std::string &content)
{
    std::vector<std::string> links;
    bool inCodeBlock = false;

    static const QRegularExpression wikiRe(R"(\[\[([^\]|]+)(?:\|[^\]]+)?\]\])");

    for (const auto &line : std::views::split(content, '\n')) {
        std::string_view sv(line.begin(), line.end());

        if (sv.starts_with("```"))
            inCodeBlock = !inCodeBlock;
        if (inCodeBlock)
            continue;

        auto qline = QString::fromUtf8(sv.data(), static_cast<int>(sv.size()));
        auto it = wikiRe.globalMatch(qline);
        while (it.hasNext()) {
            auto m = it.next();
            auto link = m.captured(1).trimmed().toStdString();
            if (!link.empty() && std::ranges::find(links, link) == links.end())
                links.push_back(std::move(link));
        }
    }

    return links;
}

void VaultManager::ensureDir(const std::filesystem::path &dir)
{
    create_directories(dir);
}

} // namespace codex
