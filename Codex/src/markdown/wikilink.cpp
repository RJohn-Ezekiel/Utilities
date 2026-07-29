#include "markdown/wikilink.h"
#include "markdown/parser.h"

#include <algorithm>
#include <cctype>

namespace codex {

WikiLinkResolver::WikiLinkResolver(const std::filesystem::path &vaultRoot)
    : m_vaultRoot(vaultRoot)
{
}

std::optional<std::filesystem::path> WikiLinkResolver::resolve(const std::string &title) const {
    auto searchDir = [&](const std::filesystem::path &dir) -> std::optional<std::filesystem::path> {
        auto dirPath = m_vaultRoot / dir;
        if (!std::filesystem::exists(dirPath))
            return std::nullopt;

        std::optional<std::filesystem::path> caseInsensitiveMatch;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(dirPath)) {
            if (!entry.is_regular_file())
                continue;
            auto stem = entry.path().stem().string();

            if (stem == title) {
                return dir / entry.path().filename();
            }

            if (caseInsensitiveMatch.has_value())
                continue;
            auto ciCompare = [](const std::string &a, const std::string &b) -> bool {
                if (a.size() != b.size())
                    return false;
                for (size_t i = 0; i < a.size(); ++i) {
                    if (std::tolower(static_cast<unsigned char>(a[i])) !=
                        std::tolower(static_cast<unsigned char>(b[i])))
                        return false;
                }
                return true;
            };
            if (ciCompare(stem, title))
                caseInsensitiveMatch = dir / entry.path().filename();
        }
        return caseInsensitiveMatch;
    };

    auto result = searchDir(vault_layout::DIR_NOTES);
    if (result)
        return result;

    result = searchDir(vault_layout::DIR_JOURNAL);
    if (result)
        return result;

    return std::nullopt;
}

std::vector<std::pair<std::string, std::optional<std::filesystem::path>>>
WikiLinkResolver::resolveAll(const std::string &content) const {
    auto links = Parser::extractWikiLinks(content);
    std::vector<std::pair<std::string, std::optional<std::filesystem::path>>> results;
    results.reserve(links.size());
    for (const auto &link : links) {
        results.emplace_back(link, resolve(link));
    }
    return results;
}

bool WikiLinkResolver::exists(const std::filesystem::path &vaultRelativePath,
                              const std::filesystem::path &vaultRoot) {
    return std::filesystem::exists(vaultRoot / vaultRelativePath);
}

} // namespace codex
