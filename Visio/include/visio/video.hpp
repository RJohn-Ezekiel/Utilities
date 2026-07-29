#pragma once

#include "visio/error.hpp"
#include "visio/types.hpp"

#include <string_view>

namespace visio {

/// Utility functions for video metadata operations.
struct VideoUtils
{
    VideoUtils() = delete;

    /// Extract a YouTube video ID from a URL or return the input unchanged
    /// if it's already an ID.
    [[nodiscard]] static std::string extractId(std::string_view input);

    /// Build a watch URL from a video ID.
    [[nodiscard]] static std::string watchUrl(std::string_view videoId);

    /// Build a short youtu.be URL from a video ID.
    [[nodiscard]] static std::string shortUrl(std::string_view videoId);

    /// Attempt to parse a raw view-count string to uint64_t.
    /// Handles formats like "1.2M", "500K", "1234".
    [[nodiscard]] static std::optional<std::uint64_t> parseViewCount(
        std::string_view viewString);

    /// Fetch video metadata via yt-dlp --dump-json.
    [[nodiscard]] static Result<Video> fetchMetadata(
        std::string_view videoId);
};

} // namespace visio
