#include <visio/client.hpp>
#include <visio/video.hpp>

#include <iostream>

int main()
{
    visio::Client yt;

    // Example 1: Channel search
    std::cout << "=== Channel Search ===\n";
    auto channels = yt.searchChannels("music");
    if (channels.hasValue()) {
        for (const auto& ch : channels.value()) {
            std::cout << "Channel: " << ch.title << '\n';
            if (!ch.subscribers.empty()) {
                std::cout << "  Subs: " << ch.subscribers << '\n';
            }
        }
    } else {
        std::cout << "Channel search unavailable: "
                  << channels.error().message() << '\n';
    }

    // Example 2: Video URL parsing
    std::cout << "\n=== URL Parsing ===\n";
    auto id = visio::VideoUtils::extractId(
        "https://www.youtube.com/watch?v=dQw4w9WgXcQ");
    std::cout << "Extracted ID: " << id << '\n';
    std::cout << "Watch URL:    " << visio::VideoUtils::watchUrl(id) << '\n';
    std::cout << "Short URL:    " << visio::VideoUtils::shortUrl(id) << '\n';

    // Example 3: Video metadata via yt-dlp
    std::cout << "\n=== Video Metadata ===\n";
    auto metadata = visio::VideoUtils::fetchMetadata(id);
    if (metadata.hasValue()) {
        const auto& v = metadata.value();
        std::cout << "Title:    " << v.title << '\n';
        std::cout << "Author:   " << v.author << '\n';
        std::cout << "Duration: " << v.duration << '\n';
        std::cout << "Views:    " << v.views << '\n';
    } else {
        std::cout << "Metadata unavailable: "
                  << metadata.error().message() << '\n';
    }

    // Example 4: Playback and download URLs
    std::cout << "\n=== Actions ===\n";
    std::cout << "Play:     yt.play(video)\n";
    std::cout << "Download: yt.download(video, \"./downloads\")\n";

    return 0;
}
