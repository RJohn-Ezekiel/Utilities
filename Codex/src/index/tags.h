#pragma once

#include "core/types.h"
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace codex {

class TagIndexer {
public:
    TagIndexer();

    void rebuild(const std::filesystem::path &vaultRoot);

    void indexNote(const std::filesystem::path &notePath, const std::vector<std::string> &tags);

    void removeNote(const std::filesystem::path &notePath);

    std::vector<std::filesystem::path> notesWithTag(const std::string &tag) const;

    std::vector<std::string> allTags() const;

    std::vector<std::pair<std::string, int>> tagCounts() const;

    bool save(const std::filesystem::path &path) const;
    bool load(const std::filesystem::path &path);

    bool isEmpty() const noexcept { return m_tagToNotes.empty(); }

private:
    std::unordered_map<std::string, std::unordered_set<std::string>> m_tagToNotes;
    std::unordered_map<std::string, std::unordered_set<std::string>> m_noteToTags;
};

} // namespace codex
