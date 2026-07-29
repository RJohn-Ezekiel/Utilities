#include "visio/search.hpp"

#include <nlohmann/json.hpp>

#include <format>

namespace visio {

std::string Search::buildSearchUrl(std::string_view query, int limit)
{
    return std::format(
        "https://www.youtube.com/results?search_query={}"
        "&sp=EgIQAQ%253D%253D&hl=en&gl=US",
        query);
}

std::string Search::buildChannelSearchUrl(std::string_view query)
{
    return std::format(
        "https://www.youtube.com/results?search_query={}"
        "&sp=EgIQAg%253D%253D&hl=en&gl=US",
        query);
}

Result<std::vector<Video>> Search::parseSearchResults(
    std::string_view json)
{
    // Forward to the full parser in client.cpp
    // This is a utility that can be used independently
    try {
        auto data = nlohmann::json::parse(json);
        std::vector<Video> videos;

        auto contents = data["contents"]
            ["twoColumnSearchResultsRenderer"]["primaryContents"]
            ["sectionListRenderer"]["contents"];

        for (const auto& section : contents) {
            if (!section.contains("itemSectionRenderer")) continue;
            auto items = section["itemSectionRenderer"]["contents"];
            for (const auto& item : items) {
                if (!item.contains("videoRenderer")) continue;
                auto& vr = item["videoRenderer"];

                Video video;
                video.id = vr.value("videoId", "");
                if (vr.contains("title") && vr["title"].contains("runs") &&
                    !vr["title"]["runs"].empty())
                {
                    video.title = vr["title"]["runs"][0].value("text", "");
                }
                if (vr.contains("longBylineText") &&
                    vr["longBylineText"].contains("runs") &&
                    !vr["longBylineText"]["runs"].empty())
                {
                    video.author = vr["longBylineText"]["runs"][0]
                        .value("text", "");
                }
                if (vr.contains("lengthText") &&
                    vr["lengthText"].contains("simpleText"))
                {
                    video.duration = vr["lengthText"]
                        .value("simpleText", "");
                }
                if (vr.contains("publishedTimeText") &&
                    vr["publishedTimeText"].contains("simpleText"))
                {
                    video.published = vr["publishedTimeText"]
                        .value("simpleText", "");
                }
                if (vr.contains("viewCountText") &&
                    vr["viewCountText"].contains("simpleText"))
                {
                    // Rough parse of view count
                    auto vs = vr["viewCountText"]
                        .value("simpleText", "");
                    // Simple parse
                    std::string numStr;
                    for (char c : vs) {
                        if (std::isdigit(c) || c == '.' || c == 'K' ||
                            c == 'M' || c == 'B')
                        {
                            numStr += c;
                        }
                    }
                    if (!numStr.empty()) {
                        double mult = 1.0;
                        if (numStr.ends_with('B')) { mult = 1e9; numStr.pop_back(); }
                        else if (numStr.ends_with('M')) { mult = 1e6; numStr.pop_back(); }
                        else if (numStr.ends_with('K')) { mult = 1e3; numStr.pop_back(); }
                        try { video.views =
                            static_cast<std::uint64_t>(
                                std::stod(numStr) * mult); }
                        catch (...) {}
                    }
                }
                if (vr.contains("thumbnail") &&
                    vr["thumbnail"].contains("thumbnails") &&
                    !vr["thumbnail"]["thumbnails"].empty())
                {
                    video.thumbnail = vr["thumbnail"]["thumbnails"]
                        .back().value("url", "");
                }

                videos.push_back(std::move(video));
            }
        }

        return videos;
    } catch (const nlohmann::json::exception& e) {
        return makeError<std::vector<Video>>(
            ErrorCode::ParseError,
            std::format("search parse error: {}", e.what()));
    }
}

Result<std::vector<Channel>> Search::parseChannelResults(
    std::string_view json)
{
    try {
        auto data = nlohmann::json::parse(json);
        std::vector<Channel> channels;

        auto contents = data["contents"]
            ["twoColumnSearchResultsRenderer"]["primaryContents"]
            ["sectionListRenderer"]["contents"];

        for (const auto& section : contents) {
            if (!section.contains("itemSectionRenderer")) continue;
            auto items = section["itemSectionRenderer"]["contents"];
            for (const auto& item : items) {
                if (!item.contains("channelRenderer")) continue;
                auto& cr = item["channelRenderer"];

                Channel ch;
                ch.channelId = cr.value("channelId", "");
                if (cr.contains("title") &&
                    cr["title"].contains("simpleText"))
                {
                    ch.title = cr["title"].value("simpleText", "");
                }
                if (cr.contains("subscriberCountText") &&
                    cr["subscriberCountText"].contains("simpleText"))
                {
                    ch.channelName = cr["subscriberCountText"]
                        .value("simpleText", "");
                }
                if (cr.contains("videoCountText") &&
                    cr["videoCountText"].contains("simpleText"))
                {
                    ch.videoCount = cr["videoCountText"]
                        .value("simpleText", "");
                }
                if (cr.contains("thumbnail") &&
                    cr["thumbnail"].contains("thumbnails") &&
                    !cr["thumbnail"]["thumbnails"].empty())
                {
                    ch.thumbnail = cr["thumbnail"]["thumbnails"]
                        .back().value("url", "");
                    if (!ch.thumbnail.empty() &&
                        !ch.thumbnail.starts_with("http"))
                    {
                        ch.thumbnail = "https:" + ch.thumbnail;
                    }
                }
                channels.push_back(std::move(ch));
            }
        }

        return channels;
    } catch (const nlohmann::json::exception& e) {
        return makeError<std::vector<Channel>>(
            ErrorCode::ParseError,
            std::format("channel parse error: {}", e.what()));
    }
}

} // namespace visio
