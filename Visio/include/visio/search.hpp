#pragma once

#include "visio/error.hpp"
#include "visio/types.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace visio {

/// Utility functions for constructing and parsing YouTube search URLs/APIs.
struct Search
{
    Search() = delete;

    /// Build a YouTube search URL.
    [[nodiscard]] static std::string buildSearchUrl(
        std::string_view query,
        int limit = 15);

    /// Build a channel search URL.
    [[nodiscard]] static std::string buildChannelSearchUrl(
        std::string_view query);

    /// Parse video search results from raw YouTube response JSON.
    [[nodiscard]] static Result<std::vector<Video>> parseSearchResults(
        std::string_view json);

    /// Parse channel search results from raw YouTube response JSON.
    [[nodiscard]] static Result<std::vector<Channel>> parseChannelResults(
        std::string_view json);
};

} // namespace visio
