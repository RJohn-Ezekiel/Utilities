#pragma once

#include "core/types.h"
#include <string>

namespace codex {

class Parser {
public:
    static Note parse(const std::string &content, const std::filesystem::path &relativePath = {});

    static std::string extractTitle(const std::string &content);

    static std::vector<std::string> extractTags(const std::string &content);

    static std::vector<std::string> extractWikiLinks(const std::string &content);

    static bool hasFrontmatter(const std::string &content);

    static std::vector<std::pair<std::string, std::string>> extractFrontmatter(const std::string &content);

    static std::string toHtml(const std::string &content);
};

} // namespace codex
