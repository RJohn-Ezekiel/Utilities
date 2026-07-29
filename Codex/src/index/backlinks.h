#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

namespace codex {

class BacklinkManager {
public:
    BacklinkManager();

    void rebuild(const std::filesystem::path &vaultRoot);

    void updateNote(const std::filesystem::path &notePath, const std::vector<std::string> &wikiLinks);

    void removeNote(const std::filesystem::path &notePath);

    std::vector<std::filesystem::path> backlinksTo(const std::filesystem::path &notePath) const;

    std::vector<std::filesystem::path> forwardLinksFrom(const std::filesystem::path &notePath) const;

    bool save(const std::filesystem::path &path) const;
    bool load(const std::filesystem::path &path);

    bool isEmpty() const noexcept { return m_graph.empty(); }

private:
    std::unordered_map<std::string, std::vector<std::string>> m_graph;

    static std::vector<std::string> scanWikiLinks(const std::filesystem::path &filePath);
};

} // namespace codex
