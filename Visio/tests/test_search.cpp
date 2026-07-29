#include <catch2/catch_test_macros.hpp>

#include <visio/search.hpp>
#include <visio/video.hpp>

using namespace visio;

TEST_CASE("Search::buildSearchUrl", "[search]")
{
    auto url = Search::buildSearchUrl("test query", 15);
    CHECK(url.find("test") != std::string::npos);
    CHECK(url.find("youtube.com/results") != std::string::npos);
}

TEST_CASE("Search::buildChannelSearchUrl", "[search]")
{
    auto url = Search::buildChannelSearchUrl("music");
    CHECK(url.find("music") != std::string::npos);
    CHECK(url.find("EgIQAg") != std::string::npos);
}

TEST_CASE("VideoUtils::extractId from various formats", "[video]")
{
    // Full URL
    auto id1 = VideoUtils::extractId(
        "https://www.youtube.com/watch?v=dQw4w9WgXcQ");
    CHECK(id1 == "dQw4w9WgXcQ");

    // Short URL
    auto id2 = VideoUtils::extractId("https://youtu.be/dQw4w9WgXcQ");
    CHECK(id2 == "dQw4w9WgXcQ");

    // Embed URL
    auto id3 = VideoUtils::extractId(
        "https://www.youtube.com/embed/dQw4w9WgXcQ");
    CHECK(id3 == "dQw4w9WgXcQ");

    // Shorts URL
    auto id4 = VideoUtils::extractId(
        "https://www.youtube.com/shorts/dQw4w9WgXcQ");
    CHECK(id4 == "dQw4w9WgXcQ");

    // Already an ID
    auto id5 = VideoUtils::extractId("dQw4w9WgXcQ");
    CHECK(id5 == "dQw4w9WgXcQ");

    // ID with special chars in URL params
    auto id6 = VideoUtils::extractId(
        "https://www.youtube.com/watch?v=dQw4w9WgXcQ&t=123");
    CHECK(id6 == "dQw4w9WgXcQ");
}

TEST_CASE("VideoUtils::watchUrl and shortUrl", "[video]")
{
    CHECK(VideoUtils::watchUrl("dQw4w9WgXcQ")
          == "https://www.youtube.com/watch?v=dQw4w9WgXcQ");
    CHECK(VideoUtils::shortUrl("dQw4w9WgXcQ")
          == "https://youtu.be/dQw4w9WgXcQ");
}

TEST_CASE("VideoUtils::parseViewCount", "[video]")
{
    auto v1 = VideoUtils::parseViewCount("1.5M views");
    REQUIRE(v1.has_value());
    CHECK(*v1 == 1'500'000);

    auto v2 = VideoUtils::parseViewCount("500K");
    REQUIRE(v2.has_value());
    CHECK(*v2 == 500'000);

    auto v3 = VideoUtils::parseViewCount("1234");
    REQUIRE(v3.has_value());
    CHECK(*v3 == 1234);

    auto v4 = VideoUtils::parseViewCount("2.3B");
    REQUIRE(v4.has_value());
    CHECK(*v4 == 2'300'000'000);

    // Empty input returns nullopt
    auto v5 = VideoUtils::parseViewCount("");
    CHECK_FALSE(v5.has_value());
}

TEST_CASE("Search::parseSearchResults with empty JSON", "[search]")
{
    auto result = Search::parseSearchResults("{}");
    CHECK(result.hasValue());
    CHECK(result.value().empty());
}

TEST_CASE("Search::parseChannelResults with empty JSON", "[search]")
{
    auto result = Search::parseChannelResults("{}");
    CHECK(result.hasValue());
    CHECK(result.value().empty());
}

TEST_CASE("Search::parseSearchResults with invalid JSON", "[search]")
{
    auto result = Search::parseSearchResults("not json");
    CHECK_FALSE(result.hasValue());
}
