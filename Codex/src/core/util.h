#pragma once

#include <filesystem>
#include <string>

namespace codex {
namespace util {

std::string sanitizeFilename(const std::string &name);

std::filesystem::path configDir();

std::filesystem::path defaultConfigPath();

} // namespace util
} // namespace codex
