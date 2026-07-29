#pragma once

#include "core/types.h"
#include <filesystem>
#include <optional>
#include <string>

namespace codex {

class WikiLinkResolver {
public:
    explicit WikiLinkResolver(const std::filesystem::path &vaultRoot);

    std::optional<std::filesystem::path> resolve(const std::string &title) const;

    std::vector<std::pair<std::string, std::optional<std::filesystem::path>>>
    resolveAll(const std::string &content) const;

    static bool exists(const std::filesystem::path &vaultRelativePath, const std::filesystem::path &vaultRoot);

private:
    std::filesystem::path m_vaultRoot;
};

} // namespace codex
