#pragma once

#include "visio/error.hpp"
#include "visio/types.hpp"

#include <string_view>
#include <vector>

namespace visio {

/// Utility functions for playlist management.
struct PlaylistUtils
{
    PlaylistUtils() = delete;

    /// Get the playlist directory path.
    [[nodiscard]] static std::string playlistDir();

    /// Ensure playlist directory exists.
    static void ensurePlaylistDir();

    /// Serialize a playlist to JSON file.
    [[nodiscard]] static Result<void> savePlaylist(
        std::string_view name,
        const std::vector<Video>& videos);

    /// Deserialize a playlist from JSON file.
    [[nodiscard]] static Result<std::vector<Video>> loadPlaylist(
        std::string_view name);

    /// List all saved playlist names.
    [[nodiscard]] static Result<std::vector<std::string>> listPlaylists();

    /// Delete a saved playlist file.
    [[nodiscard]] static Result<void> deletePlaylist(
        std::string_view name);
};

} // namespace visio
