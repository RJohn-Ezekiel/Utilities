#include "media/media.h"

#include <fstream>
#include <algorithm>

namespace codex {

MediaManager::MediaManager(const std::filesystem::path &vaultRoot, MediaInsert insertBehavior)
    : m_vaultRoot(vaultRoot), m_behavior(insertBehavior) {}

std::optional<std::filesystem::path> MediaManager::importFile(const std::filesystem::path &sourceFile) {
    if (!std::filesystem::exists(sourceFile))
        return std::nullopt;

    auto relDir = categoryDir(sourceFile);
    auto targetDir = m_vaultRoot / relDir;

    std::error_code ec;
    std::filesystem::create_directories(targetDir, ec);
    if (ec)
        return std::nullopt;

    auto targetRel = relDir / sourceFile.filename();
    auto targetAbs = m_vaultRoot / targetRel;

    if (std::filesystem::exists(targetAbs)) {
        auto stem = sourceFile.stem();
        auto ext = sourceFile.extension();
        int counter = 1;
        do {
            auto newName = stem.string() + "_" + std::to_string(counter) + ext.string();
            targetRel = relDir / newName;
            targetAbs = m_vaultRoot / targetRel;
            ++counter;
        } while (std::filesystem::exists(targetAbs));
    }

    if (m_behavior == MediaInsert::CopyToVault || m_behavior == MediaInsert::Absolute) {
        std::filesystem::copy(sourceFile, targetAbs, std::filesystem::copy_options::none, ec);
        if (ec)
            return std::nullopt;
    }

    if (m_behavior == MediaInsert::Absolute)
        return targetAbs;

    return targetRel;
}

std::filesystem::path MediaManager::categoryDir(const std::filesystem::path &filePath) {
    auto ext = filePath.extension().string();
    std::string lower;
    lower.reserve(ext.size());
    for (auto c : ext)
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (lower == ".png" || lower == ".jpg" || lower == ".jpeg" ||
        lower == ".gif" || lower == ".bmp" || lower == ".svg")
        return vault_layout::DIR_IMAGES;
    if (lower == ".mp4" || lower == ".avi" || lower == ".mkv" || lower == ".mov")
        return vault_layout::DIR_VIDEOS;
    if (lower == ".mp3" || lower == ".wav" || lower == ".ogg" || lower == ".flac")
        return vault_layout::DIR_AUDIO;
    return vault_layout::DIR_FILES;
}

bool MediaManager::isImage(const std::filesystem::path &p) {
    auto dir = categoryDir(p);
    return dir == vault_layout::DIR_IMAGES;
}

bool MediaManager::isVideo(const std::filesystem::path &p) {
    auto dir = categoryDir(p);
    return dir == vault_layout::DIR_VIDEOS;
}

bool MediaManager::isAudio(const std::filesystem::path &p) {
    auto dir = categoryDir(p);
    return dir == vault_layout::DIR_AUDIO;
}

} // namespace codex
