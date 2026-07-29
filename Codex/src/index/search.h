#pragma once

#include "core/types.h"
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>

namespace codex {

class SearchEngine {
public:
    SearchEngine();

    void rebuild(const std::filesystem::path &vaultRoot);

    void indexNote(const Note &note, const std::filesystem::path &vaultRoot);

    void removeNote(const std::filesystem::path &relativePath);

    std::vector<SearchResult> query(const std::string &searchTerm) const;

    bool save(const std::filesystem::path &path) const;
    bool load(const std::filesystem::path &path);

    bool isEmpty() const noexcept { return m_entries.empty(); }

private:
    struct IndexEntry {
        std::filesystem::path path;
        std::string title;
        std::string content;
        std::vector<std::string> tags;
        std::vector<std::string> wikiLinks;
    };

    std::unordered_map<std::string, IndexEntry> m_entries;

    float calculateRelevance(const IndexEntry &entry, const std::string &term) const;
    std::string snippet(const std::string &content, const std::string &term, int contextChars = 60) const;
};

} // namespace codex
