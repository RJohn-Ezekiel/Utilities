#include <catch2/catch_test_macros.hpp>

#include <visio/error.hpp>
#include <visio/types.hpp>

using namespace visio;

TEST_CASE("Video default construction", "[types]")
{
    Video v;
    CHECK(v.id.empty());
    CHECK(v.title.empty());
    CHECK(v.author.empty());
    CHECK(v.duration.empty());
    CHECK(v.views == 0);
    CHECK(v.published.empty());
    CHECK(v.thumbnail.empty());
    CHECK(v.description.empty());
}

TEST_CASE("Video with values", "[types]")
{
    Video v;
    v.id = "dQw4w9WgXcQ";
    v.title = "Never Gonna Give You Up";
    v.author = "Rick Astley";
    v.duration = "3:32";
    v.views = 1'500'000'000;
    v.published = "15 years ago";
    v.thumbnail = "https://i.ytimg.com/vi/dQw4w9WgXcQ/maxresdefault.jpg";
    v.description = "The official video for...";

    CHECK(v.id == "dQw4w9WgXcQ");
    CHECK(v.title == "Never Gonna Give You Up");
    CHECK(v.author == "Rick Astley");
    CHECK(v.duration == "3:32");
    CHECK(v.views == 1'500'000'000);
    CHECK(v.published == "15 years ago");
    CHECK(v.thumbnail == "https://i.ytimg.com/vi/dQw4w9WgXcQ/maxresdefault.jpg");
    CHECK(v.description == "The official video for...");
}

TEST_CASE("Channel default construction", "[types]")
{
    Channel c;
    CHECK(c.channelId.empty());
    CHECK(c.title.empty());
    CHECK(c.channelName.empty());
    CHECK(c.thumbnail.empty());
    CHECK(c.subscribers.empty());
    CHECK(c.videoCount.empty());
}

TEST_CASE("Error construction and what", "[types]")
{
    Error err(ErrorCode::NetworkError, "connection refused");
    CHECK(err.code() == ErrorCode::NetworkError);
    CHECK(err.message() == "connection refused");
    CHECK(err.what().find("connection refused") != std::string::npos);
}

TEST_CASE("ErrorCode toString", "[types]")
{
    CHECK(toString(ErrorCode::Ok) == "Ok");
    CHECK(toString(ErrorCode::NetworkError) == "network error");
    CHECK(toString(ErrorCode::ParseError) == "parse error");
    CHECK(toString(ErrorCode::NotFound) == "not found");
    CHECK(toString(ErrorCode::InvalidInput) == "invalid input");
}

TEST_CASE("Result with value", "[types]")
{
    Result<int> r(42);
    CHECK(r.hasValue());
    CHECK(static_cast<bool>(r));
    CHECK(r.value() == 42);
    CHECK(r.valueOr(0) == 42);
}

TEST_CASE("Result with error", "[types]")
{
    Result<int> r(Error(ErrorCode::NotFound, "not found"));
    CHECK_FALSE(r.hasValue());
    CHECK_FALSE(static_cast<bool>(r));
    CHECK(r.valueOr(-1) == -1);
    CHECK_THROWS_AS(r.value(), VisioException);
}

TEST_CASE("Result move semantics", "[types]")
{
    Result<std::string> r1(std::string("hello"));
    Result<std::string> r2(std::move(r1));
    CHECK(r2.hasValue());
    CHECK(r2.value() == "hello");
}

TEST_CASE("Result void with value", "[types]")
{
    Result<void> r;
    CHECK(r.hasValue());
    CHECK_NOTHROW(r.value());
}

TEST_CASE("Result void with error", "[types]")
{
    Result<void> r(Error(ErrorCode::NetworkError, "fail"));
    CHECK_FALSE(r.hasValue());
    CHECK_THROWS_AS(r.value(), VisioException);
}

TEST_CASE("VisioException", "[types]")
{
    Error err(ErrorCode::ParseError, "bad json");
    VisioException ex(err);
    CHECK(ex.error().code() == ErrorCode::ParseError);
    CHECK(std::string(ex.what()).find("bad json") != std::string::npos);
}

TEST_CASE("Quality enum", "[types]")
{
    static_assert(std::is_same_v<std::underlying_type_t<Quality>, int>);
}
