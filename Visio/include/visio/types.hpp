#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace visio {

/// Represents a YouTube video with its metadata.
struct Video
{
    std::string            id;          ///< YouTube video ID (e.g., "dQw4w9WgXcQ")
    std::string            title;       ///< Video title
    std::string            author;      ///< Channel/uploader name
    std::string            duration;    ///< Human-readable duration (e.g., "4:20")
    std::uint64_t          views{0};    ///< View count
    std::string            published;   ///< Relative publish time (e.g., "2 years ago")
    std::string            thumbnail;   ///< URL to the best available thumbnail
    std::string            description; ///< Video description (may be empty)
};

/// Represents a YouTube channel.
struct Channel
{
    std::string channelId;   ///< YouTube channel ID
    std::string title;       ///< Channel display name
    std::string channelName; ///< Channel handle/username
    std::string thumbnail;   ///< URL to channel avatar thumbnail
    std::string subscribers; ///< Human-readable subscriber count
    std::string videoCount;  ///< Human-readable video count
};

/// Represents a saved playlist.
struct Playlist
{
    std::string      name;   ///< Playlist name
    std::vector<Video> videos; ///< Videos in the playlist
};

/// Search result modes.
enum class SearchType
{
    Video,   ///< Search for videos
    Channel, ///< Search for channels
};

/// Quality preset for playback/download.
enum class Quality
{
    Best,
    Worst,
    P720,
    P1080,
    P2160,
    AudioOnly,
};

} // namespace visio
