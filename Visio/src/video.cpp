#include "visio/video.hpp"
#include "http_util.hpp"

#include <nlohmann/json.hpp>

#include <format>
#include <regex>

namespace visio {

std::string VideoUtils::extractId(std::string_view input)
{
    // Check if it's already a short ID (11 chars, alphanumeric + _-)
    if (input.size() == 11 &&
        std::all_of(input.begin(), input.end(), [](char c) {
            return std::isalnum(c) || c == '_' || c == '-';
        }))
    {
        return std::string(input);
    }

    // Try to extract from URL
    std::string s(input);
    std::smatch match;

    // youtu.be/<id>
    if (std::regex_search(s, match,
        std::regex(R"(youtu\.be/([a-zA-Z0-9_-]{11}))")))
    {
        return match[1];
    }

    // youtube.com/watch?v=<id>
    if (std::regex_search(s, match,
        std::regex(R"([?&]v=([a-zA-Z0-9_-]{11}))")))
    {
        return match[1];
    }

    // youtube.com/embed/<id>
    if (std::regex_search(s, match,
        std::regex(R"(embed/([a-zA-Z0-9_-]{11}))")))
    {
        return match[1];
    }

    // youtube.com/shorts/<id>
    if (std::regex_search(s, match,
        std::regex(R"(shorts/([a-zA-Z0-9_-]{11}))")))
    {
        return match[1];
    }

    return std::string(input);
}

std::string VideoUtils::watchUrl(std::string_view videoId)
{
    return std::format("https://www.youtube.com/watch?v={}", videoId);
}

std::string VideoUtils::shortUrl(std::string_view videoId)
{
    return std::format("https://youtu.be/{}", videoId);
}

std::optional<std::uint64_t> VideoUtils::parseViewCount(
    std::string_view viewString)
{
    std::string s(viewString);
    // Extract numeric part with optional suffix
    std::string numPart;
    char suffix = 0;
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        if (*it == 'B' || *it == 'M' || *it == 'K') {
            suffix = *it;
            break;
        }
    }

    for (char c : s) {
        if (std::isdigit(c) || c == '.') {
            numPart += c;
        }
    }

    if (numPart.empty()) return std::nullopt;

    double multiplier = 1.0;
    if (suffix == 'B') multiplier = 1'000'000'000;
    else if (suffix == 'M') multiplier = 1'000'000;
    else if (suffix == 'K') multiplier = 1'000;

    try {
        double val = std::stod(numPart) * multiplier;
        return static_cast<std::uint64_t>(val);
    } catch (...) {
        return std::nullopt;
    }
}

Result<Video> VideoUtils::fetchMetadata(std::string_view videoId)
{
    auto cmd = std::format(
        "yt-dlp --dump-json --no-warnings "
        "https://www.youtube.com/watch?v={}",
        videoId);

    auto output = detail::exec(cmd);
    if (!output.hasValue()) {
        return makeError<Video>(
            ErrorCode::NetworkError,
            std::format("yt-dlp failed for '{}': {}",
                        videoId, output.error().message()));
    }

    try {
        auto data = nlohmann::json::parse(output.value());
        Video v;
        v.id = data.value("id", std::string(videoId));
        v.title = data.value("title", "");
        v.author = data.value("channel", "");
        v.description = data.value("description", "");
        v.views = data.value("view_count", 0ULL);
        v.thumbnail = data.value("thumbnail", "");

        if (data.contains("duration")) {
            auto secs = data["duration"].get<double>();
            auto total = static_cast<int>(secs);
            auto mins = total / 60;
            auto secsLeft = total % 60;
            if (mins >= 60) {
                auto hrs = mins / 60;
                mins %= 60;
                v.duration = std::format("{}:{:02}:{:02}", hrs, mins, secsLeft);
            } else {
                v.duration = std::format("{}:{:02}", mins, secsLeft);
            }
        }

        return v;
    } catch (const nlohmann::json::exception& e) {
        return makeError<Video>(
            ErrorCode::ParseError,
            std::format("yt-dlp output parse error: {}", e.what()));
    }
}

} // namespace visio
