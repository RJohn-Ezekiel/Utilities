#pragma once

#include "visio/channel.hpp"
#include "visio/error.hpp"
#include "visio/playlist.hpp"
#include "visio/search.hpp"
#include "visio/types.hpp"
#include "visio/video.hpp"

#include <memory>
#include <string_view>

namespace visio {

/// Main entry point for interacting with YouTube.
///
/// Client manages HTTP requests, caching, and external process
/// invocations (curl, yt-dlp, mpv) to provide a complete YouTube
/// browsing experience from C++.
///
/// Usage:
/// @code
///   visio::Client yt;
///   auto results = yt.search("modern c++ tutorial");
///   for (const auto& v : results.value()) {
///       std::cout << v.title << '\n';
///   }
/// @endcode
class Client
{
public:
    Client();
    ~Client() noexcept;

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    /// Search YouTube for videos matching @p query.
    /// @param query  Search string.
    /// @param limit  Maximum number of results (default 15).
    [[nodiscard]] Result<std::vector<Video>> search(
        std::string_view query,
        int limit = 15);

    /// Search for channels matching @p query.
    [[nodiscard]] Result<std::vector<Channel>> searchChannels(
        std::string_view query);

    /// Get detailed information about a video by its ID.
    [[nodiscard]] Result<Video> getVideo(std::string_view videoId);

    /// Get recent videos from a channel.
    [[nodiscard]] Result<std::vector<Video>> getChannelVideos(
        std::string_view channelId,
        int limit = 15);

    /// Get videos from the subscription feed.
    [[nodiscard]] Result<std::vector<Video>> getFeed(int limit = 15);

    // ---- Playback & Download ----

    /// Play a video using mpv.
    bool play(const Video& video, Quality quality = Quality::Best);

    /// Download a video using yt-dlp.
    Result<void> download(
        const Video& video,
        std::string_view directory = {},
        Quality quality = Quality::Best);

    /// Download audio-only (MP3) with thumbnail and metadata.
    Result<void> downloadAudio(
        const Video& video,
        std::string_view directory = {});

    /// Download multiple videos as a batch (e.g. playlist).
    Result<void> downloadMultiple(
        const std::vector<Video>& videos,
        std::string_view directory = {},
        Quality quality = Quality::Best);

    // ---- History ----

    /// Retrieve watch history.
    [[nodiscard]] Result<std::vector<Video>> getHistory();

    /// Clear all watch history.
    void clearHistory();

    /// Remove a single entry from history by index.
    void removeHistoryEntry(std::size_t index);

    // ---- Subscriptions ----

    /// Add a channel to subscriptions.
    Result<void> subscribe(const Channel& channel);

    /// Remove a channel from subscriptions.
    Result<void> unsubscribe(std::string_view channelId);

    /// List all subscribed channels.
    [[nodiscard]] Result<std::vector<Channel>> getSubscriptions();

    /// Import subscriptions from a browser's cookies.
    Result<void> importSubscriptions(std::string_view browserName);

    // ---- Queue ----

    /// Add a video to the playback queue.
    void addToQueue(const Video& video);

    /// Get all queued videos.
    [[nodiscard]] Result<std::vector<Video>> getQueue();

    /// Clear the playback queue.
    void clearQueue();

    // ---- Playlists ----

    /// List saved playlist names.
    [[nodiscard]] Result<std::vector<std::string>> listPlaylists();

    /// Load a saved playlist by name.
    [[nodiscard]] Result<std::vector<Video>> loadPlaylist(
        std::string_view name);

    /// Save the current queue as a named playlist.
    Result<void> saveQueueAsPlaylist(std::string_view name);

    /// Add a single video to an existing playlist.
    Result<void> addToPlaylist(std::string_view name, const Video& video);

    /// Delete a saved playlist by name.
    Result<void> deletePlaylist(std::string_view name);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace visio
