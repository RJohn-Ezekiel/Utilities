#pragma once

#include "visio/error.hpp"
#include "visio/types.hpp"

#include <string_view>
#include <vector>

namespace visio {

/// Utility functions for channel operations.
struct ChannelUtils
{
    ChannelUtils() = delete;

    /// Build a channel videos page URL.
    [[nodiscard]] static std::string channelVideosUrl(
        std::string_view channelId);

    /// Parse channel video listing from raw YouTube response.
    [[nodiscard]] static Result<std::vector<Video>> parseChannelVideos(
        std::string_view json);

    /// Extract channel info from search result JSON entry.
    [[nodiscard]] static Result<Channel> parseChannelInfo(
        std::string_view jsonEntry);
};

} // namespace visio
