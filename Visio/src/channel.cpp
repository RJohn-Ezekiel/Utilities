#include "visio/channel.hpp"

#include <nlohmann/json.hpp>

#include <format>

namespace visio {

std::string ChannelUtils::channelVideosUrl(std::string_view channelId)
{
    return std::format("https://www.youtube.com/{}/videos", channelId);
}

Result<std::vector<Video>> ChannelUtils::parseChannelVideos(
    std::string_view json)
{
    try {
        auto data = nlohmann::json::parse(json);
        std::vector<Video> videos;

        auto tabs = data["contents"]
            ["twoColumnBrowseResultsRenderer"]["tabs"];
        for (const auto& tab : tabs) {
            if (!tab.contains("tabRenderer")) continue;
            auto& tr = tab["tabRenderer"];
            if (!tr.contains("content")) continue;
            if (!tr["content"].contains("richGridRenderer")) continue;
            auto contents = tr["content"]["richGridRenderer"]["contents"];

            for (const auto& item : contents) {
                if (!item.contains("richItemRenderer")) continue;
                auto& ri = item["richItemRenderer"]["content"];
                if (!ri.contains("videoRenderer")) continue;
                auto& vr = ri["videoRenderer"];

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
                    auto vs = vr["viewCountText"]
                        .value("simpleText", "");
                    std::string numStr;
                    for (char c : vs) {
                        if (std::isdigit(c) || c == '.' || c == 'K' ||
                            c == 'M' || c == 'B')
                            numStr += c;
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
            if (!videos.empty()) break;
        }

        return videos;
    } catch (const nlohmann::json::exception& e) {
        return makeError<std::vector<Video>>(
            ErrorCode::ParseError,
            std::format("channel video parse error: {}", e.what()));
    }
}

Result<Channel> ChannelUtils::parseChannelInfo(std::string_view jsonEntry)
{
    try {
        auto data = nlohmann::json::parse(jsonEntry);

        Channel ch;
        ch.channelId = data.value("channelId", "");
        if (data.contains("title") && data["title"].contains("simpleText")) {
            ch.title = data["title"].value("simpleText", "");
        }
        if (data.contains("subscriberCountText") &&
            data["subscriberCountText"].contains("simpleText"))
        {
            ch.channelName = data["subscriberCountText"]
                .value("simpleText", "");
        }
        if (data.contains("videoCountText") &&
            data["videoCountText"].contains("simpleText"))
        {
            ch.videoCount = data["videoCountText"]
                .value("simpleText", "");
        }
        if (data.contains("thumbnail") &&
            data["thumbnail"].contains("thumbnails") &&
            !data["thumbnail"]["thumbnails"].empty())
        {
            ch.thumbnail = data["thumbnail"]["thumbnails"]
                .back().value("url", "");
            if (!ch.thumbnail.empty() &&
                !ch.thumbnail.starts_with("http"))
            {
                ch.thumbnail = "https:" + ch.thumbnail;
            }
        }

        return ch;
    } catch (const nlohmann::json::exception& e) {
        return makeError<Channel>(
            ErrorCode::ParseError,
            std::format("channel info parse error: {}", e.what()));
    }
}

} // namespace visio
