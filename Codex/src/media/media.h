#pragma once

#include "core/types.h"
#include <filesystem>
#include <string>
#include <optional>

namespace codex {

class MediaManager {
public:
    explicit MediaManager(const std::filesystem::path &vaultRoot, MediaInsert insertBehavior = MediaInsert::CopyToVault);

    std::optional<std::filesystem::path> importFile(const std::filesystem::path &sourceFile);

    static std::filesystem::path categoryDir(const std::filesystem::path &filePath);

    static bool isImage(const std::filesystem::path &p);
    static bool isVideo(const std::filesystem::path &p);
    static bool isAudio(const std::filesystem::path &p);

private:
    std::filesystem::path m_vaultRoot;
    MediaInsert m_behavior;
};

} // namespace codex
