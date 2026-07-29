#include "core/util.h"

#include <cstdlib>

namespace codex {
namespace util {

std::string sanitizeFilename(const std::string &name)
{
    std::string result = name;
    for (auto &c : result) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|')
            c = '_';
    }
    if (result.empty())
        result = "untitled";
    return result;
}

std::filesystem::path configDir()
{
    auto home = std::getenv("HOME")
        ? std::filesystem::path(std::getenv("HOME"))
        : std::filesystem::path("/tmp");
    return home / ".config" / "codex";
}

std::filesystem::path defaultConfigPath()
{
    return configDir() / "config.json";
}

} // namespace util
} // namespace codex
