#pragma once

#include "core/types.h"
#include <filesystem>
#include <string>
#include <vector>

namespace codex {

class ExportManager {
public:
    enum Format { Html, Markdown, PlainText };

    explicit ExportManager(const std::filesystem::path &vaultRoot);

    bool exportNote(const Note &note, const std::filesystem::path &outputDir, Format fmt = Html);
    bool exportMultiple(const std::vector<Note> &notes, const std::filesystem::path &outputDir, Format fmt = Html);
    bool exportAll(const std::vector<Note> &allNotes, const std::filesystem::path &outputDir, Format fmt = Html);

    static std::string stripMarkdown(const std::string &markdown);

private:
    std::filesystem::path m_vaultRoot;

    static std::string renderHtml(const Note &note, const std::string &bodyHtml);
    static std::string markdownToHtml(const std::string &markdown);
    static std::string htmlTemplate();
    static std::string sanitizeFilename(const std::string &name);
    static std::string extensionFor(Format fmt);
};

} // namespace codex
