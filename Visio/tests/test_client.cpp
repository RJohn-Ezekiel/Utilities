#include <catch2/catch_test_macros.hpp>

#include <visio/client.hpp>

#include <thread>

using namespace visio;

TEST_CASE("Client default construction", "[client]")
{
    Client yt;
    SUCCEED("Client constructed without exception");
}

TEST_CASE("Client move construction", "[client]")
{
    Client yt1;
    Client yt2(std::move(yt1));
    SUCCEED("Client move-constructed without exception");
}

TEST_CASE("Client move assignment", "[client]")
{
    Client yt1;
    Client yt2;
    yt2 = std::move(yt1);
    SUCCEED("Client move-assigned without exception");
}

TEST_CASE("Client can be destroyed", "[client]")
{
    auto* yt = new Client();
    delete yt;
    SUCCEED("Client destroyed without memory issues");
}

TEST_CASE("Client handles empty search gracefully", "[client]")
{
    Client yt;
    auto results = yt.search("");
    // Should either return error or empty results
    if (results.hasValue()) {
        CHECK(results.value().empty());
    } else {
        CHECK((results.error().code() == ErrorCode::InvalidInput ||
               results.error().code() == ErrorCode::NetworkError));
    }
}

TEST_CASE("Client search with very long query", "[client]")
{
    Client yt;
    std::string longQuery(1000, 'a');
    auto results = yt.search(longQuery, 5);
    // Should handle gracefully
    SUCCEED("Client handled long query without crash");
}

TEST_CASE("Client history round-trip", "[client]")
{
    Client yt;

    // History should never fail
    auto history = yt.getHistory();
    CHECK(history.hasValue());

    // Should be clean initially
    yt.clearHistory();
    history = yt.getHistory();
    CHECK(history.hasValue());
    CHECK(history.value().empty());
}

TEST_CASE("Client clearHistory on fresh client", "[client]")
{
    Client yt;
    CHECK_NOTHROW(yt.clearHistory());
}

TEST_CASE("Client removeHistoryEntry out of bounds", "[client]")
{
    Client yt;
    CHECK_NOTHROW(yt.removeHistoryEntry(999));
}

TEST_CASE("Client queue operations", "[client]")
{
    Client yt;

    // Queue should start empty
    auto queue = yt.getQueue();
    CHECK(queue.hasValue());

    yt.clearQueue();
    queue = yt.getQueue();
    CHECK(queue.hasValue());
    CHECK(queue.value().empty());

    // Add a video to queue
    Video v;
    v.id = "test123";
    v.title = "Test Video";
    v.author = "Tester";
    v.duration = "1:23";
    v.views = 100;
    yt.addToQueue(v);

    queue = yt.getQueue();
    REQUIRE(queue.hasValue());
    CHECK(queue.value().size() == 1);
    CHECK(queue.value()[0].id == "test123");

    yt.clearQueue();
    queue = yt.getQueue();
    CHECK(queue.hasValue());
    CHECK(queue.value().empty());
}

TEST_CASE("Client subscription round-trip", "[client]")
{
    Client yt;

    // Get subs (should be empty)
    auto subs = yt.getSubscriptions();
    CHECK(subs.hasValue());

    // Subscribe to a channel
    Channel ch;
    ch.channelId = "UC_TestChannel123";
    ch.title = "Test Channel";
    ch.channelName = "@testchannel";
    ch.thumbnail = "https://example.com/thumb.jpg";
    ch.subscribers = "1.2K";

    auto subResult = yt.subscribe(ch);
    CHECK(subResult.hasValue());

    // Verify subscription exists
    subs = yt.getSubscriptions();
    REQUIRE(subs.hasValue());
    bool found = false;
    for (const auto& s : subs.value()) {
        if (s.channelId == "UC_TestChannel123") {
            found = true;
            CHECK(s.title == "Test Channel");
            break;
        }
    }
    CHECK(found);

    // Unsubscribe
    auto unsubResult = yt.unsubscribe("UC_TestChannel123");
    CHECK(unsubResult.hasValue());

    // Verify removed
    subs = yt.getSubscriptions();
    REQUIRE(subs.hasValue());
    found = false;
    for (const auto& s : subs.value()) {
        if (s.channelId == "UC_TestChannel123") {
            found = true;
            break;
        }
    }
    CHECK_FALSE(found);
}

TEST_CASE("Client playlists round-trip", "[client]")
{
    Client yt;

    // List should be empty
    auto lists = yt.listPlaylists();
    CHECK(lists.hasValue());

    // Add video to queue and save as playlist
    Video v;
    v.id = "pl_test1";
    v.title = "Playlist Test";
    v.author = "Tester";
    v.duration = "5:00";
    v.views = 500;
    yt.addToQueue(v);

    auto saveResult = yt.saveQueueAsPlaylist("test_playlist");
    CHECK(saveResult.hasValue());

    // List should have it
    lists = yt.listPlaylists();
    REQUIRE(lists.hasValue());
    bool found = false;
    for (const auto& name : lists.value()) {
        if (name == "test_playlist") {
            found = true;
            break;
        }
    }
    CHECK(found);

    // Load the playlist
    auto plVideos = yt.loadPlaylist("test_playlist");
    REQUIRE(plVideos.hasValue());
    CHECK(plVideos.value().size() == 1);
    CHECK(plVideos.value()[0].id == "pl_test1");

    // Delete the playlist
    auto delResult = yt.deletePlaylist("test_playlist");
    CHECK(delResult.hasValue());

    // Verify deleted
    lists = yt.listPlaylists();
    REQUIRE(lists.hasValue());
    found = false;
    for (const auto& name : lists.value()) {
        if (name == "test_playlist") {
            found = true;
            break;
        }
    }
    CHECK_FALSE(found);

    yt.clearQueue();
}

TEST_CASE("Client is movable but not copyable", "[client]")
{
    static_assert(!std::is_copy_constructible_v<Client>);
    static_assert(!std::is_copy_assignable_v<Client>);
    static_assert(std::is_move_constructible_v<Client>);
    static_assert(std::is_move_assignable_v<Client>);
}

TEST_CASE("Multiple clients can coexist", "[client]")
{
    Client yt1;
    Client yt2;
    Client yt3;

    yt1.clearHistory();
    yt2.clearHistory();
    yt3.clearHistory();

    SUCCEED("Multiple clients coexist without conflict");
}

TEST_CASE("Client thread safety basic", "[client]")
{
    Client yt;

    std::thread t1([&]() {
        yt.clearHistory();
    });
    std::thread t2([&]() {
        yt.clearQueue();
    });

    t1.join();
    t2.join();

    SUCCEED("Concurrent operations did not crash");
}
