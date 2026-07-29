#include "visio/playlist.hpp"
#include "http_util.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>

namespace visio {

std::string PlaylistUtils::playlistDir()
{
    return detail::configDir() + "/playlists";
}

void PlaylistUtils::ensurePlaylistDir()
{
    std::filesystem::create_directories(playlistDir());
}

Result<void> PlaylistUtils::savePlaylist(
    std::string_view name,
    const std::vector<Video>& videos)
{
    ensurePlaylistDir();
    auto path = playlistDir() + "/" + std::string(name) + ".json";

    nlohmann::json data = nlohmann::json::array();
    for (const auto& v : videos) {
        data.push_back({
            {"id", v.id},
            {"title", v.title},
            {"author", v.author},
            {"duration", v.duration},
            {"views", v.views},
            {"published", v.published},
            {"thumbnail", v.thumbnail},
            {"description", v.description},
        });
    }

    std::ofstream ofs(path);
    if (!ofs) {
        return Result<void>(Error(
            ErrorCode::PlaylistError,
            std::format("failed to write playlist '{}'", name)));
    }
    ofs << data.dump(2);
    return Result<void>{};
}

Result<std::vector<Video>> PlaylistUtils::loadPlaylist(
    std::string_view name)
{
    auto path = playlistDir() + "/" + std::string(name) + ".json";
    std::ifstream ifs(path);
    if (!ifs) {
        return makeError<std::vector<Video>>(
            ErrorCode::NotFound,
            std::format("playlist '{}' not found", name));
    }

    try {
        nlohmann::json data;
        ifs >> data;
        std::vector<Video> videos;
        for (const auto& item : data) {
            Video v;
            v.id = item.value("id", "");
            v.title = item.value("title", "");
            v.author = item.value("author", "");
            v.duration = item.value("duration", "");
            v.views = item.value("views", 0ULL);
            v.published = item.value("published", "");
            v.thumbnail = item.value("thumbnail", "");
            videos.push_back(std::move(v));
        }
        return videos;
    } catch (const nlohmann::json::exception& e) {
        return makeError<std::vector<Video>>(
            ErrorCode::ParseError,
            std::format("playlist parse error: {}", e.what()));
    }
}

Result<std::vector<std::string>> PlaylistUtils::listPlaylists()
{
    ensurePlaylistDir();
    std::vector<std::string> names;
    try {
        for (const auto& entry :
             std::filesystem::directory_iterator(playlistDir()))
        {
            if (entry.path().extension() == ".json") {
                names.push_back(entry.path().stem().string());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        return makeError<std::vector<std::string>>(
            ErrorCode::PlaylistError, e.what());
    }
    std::sort(names.begin(), names.end());
    return names;
}

Result<void> PlaylistUtils::deletePlaylist(std::string_view name)
{
    auto path = playlistDir() + "/" + std::string(name) + ".json";
    try {
        if (!std::filesystem::remove(path)) {
            return Result<void>(Error(
                ErrorCode::NotFound,
                std::format("playlist '{}' not found", name)));
        }
    } catch (const std::filesystem::filesystem_error& e) {
        return Result<void>(Error(
            ErrorCode::PlaylistError, e.what()));
    }
    return Result<void>{};
}

} // namespace visio
