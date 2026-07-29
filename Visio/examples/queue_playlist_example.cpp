#include <visio/client.hpp>

#include <iostream>

int main()
{
    visio::Client yt;

    // Build a queue of videos
    yt.clearQueue();

    visio::Video v1;
    v1.id = "video1";
    v1.title = "First Video";
    v1.author = "Creator A";
    v1.duration = "10:00";
    v1.views = 1000;

    visio::Video v2;
    v2.id = "video2";
    v2.title = "Second Video";
    v2.author = "Creator B";
    v2.duration = "5:30";
    v2.views = 500;

    yt.addToQueue(v1);
    yt.addToQueue(v2);

    // Show queue
    auto queue = yt.getQueue();
    if (queue.hasValue()) {
        std::cout << "Queue (" << queue.value().size() << " videos):\n";
        for (const auto& v : queue.value()) {
            std::cout << "  - " << v.title << " by " << v.author << '\n';
        }
    }

    // Save as playlist
    auto saveResult = yt.saveQueueAsPlaylist("my_favorites");
    if (saveResult.hasValue()) {
        std::cout << "\nSaved queue as playlist 'my_favorites'\n";
    }

    // List playlists
    auto lists = yt.listPlaylists();
    if (lists.hasValue()) {
        std::cout << "\nSaved playlists:\n";
        for (const auto& name : lists.value()) {
            std::cout << "  - " << name << '\n';
        }
    }

    // Load and show the playlist
    auto pl = yt.loadPlaylist("my_favorites");
    if (pl.hasValue()) {
        std::cout << "\nLoaded playlist 'my_favorites':\n";
        for (const auto& v : pl.value()) {
            std::cout << "  - " << v.title << '\n';
        }
    }

    // Clean up
    yt.deletePlaylist("my_favorites");
    yt.clearQueue();

    std::cout << "\nDone.\n";
    return 0;
}
