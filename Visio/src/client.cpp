#include "visio/client.hpp"
#include "http_util.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <random>
#include <set>
#include <thread>
namespace visio {

namespace {

/// Check if a JS runtime (deno/node) is available for yt-dlp (cached)
const std::string& jsRuntimeFlag()
{
    static std::string flag = []() -> std::string {
        auto check = [](const char* name) {
            auto cmd = std::format("command -v {} >/dev/null 2>&1", name);
            return std::system(cmd.c_str()) == 0;
        };
        if (check("deno")) return " --js-runtime deno";
        if (check("node")) return " --js-runtime node";
        if (check("nodejs")) return " --js-runtime node";
        return "";
    }();
    return flag;
}

using json = nlohmann::json;

constexpr std::string_view kUserAgent =
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36";
constexpr std::string_view kYtApiKey = "AIzaSyAO90d0o_cimLECsGBARHaB_YvqXMCm5Bk";
constexpr std::string_view kYtApiUrl =
    "https://www.youtube.com/youtubei/v1/search";

[[nodiscard]] std::string historyFile()
{
    return detail::cacheDir() + "/history.json";
}

[[nodiscard]] std::string subFile()
{
    return detail::configDir() + "/sub.json";
}

[[nodiscard]] std::string queueFile()
{
    return detail::cacheDir() + "/queue.json";
}

[[nodiscard]] std::string cacheFile(std::string_view key)
{
    return detail::cacheDir() + "/" + std::string(key) + ".json";
}

void ensureDir(std::string_view path)
{
    std::filesystem::create_directories(std::string(path));
}

void ensureFile(std::string_view path, std::string_view defaultContent = "[]")
{
    if (!std::filesystem::exists(std::string(path))) {
        std::ofstream ofs{std::string(path)};
        ofs << defaultContent;
    }
}

[[nodiscard]] std::optional<json> readJsonFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) return std::nullopt;
    try {
        json j;
        ifs >> j;
        return j;
    } catch (...) {
        return std::nullopt;
    }
}

bool writeJsonFile(const std::string& path, const json& data)
{
    std::ofstream ofs(path);
    if (!ofs) return false;
    ofs << data.dump();
    return true;
}

[[nodiscard]] std::string sha256(std::string_view input)
{
    auto result = detail::exec(
        std::format("printf '%s' '{}' | sha256sum | cut -d' ' -f1",
                     input));
    if (result.hasValue()) {
        auto s = std::move(result).value();
        if (!s.empty() && s.back() == '\n') s.pop_back();
        return s;
    }
    // Fallback: use input as-is (hashed poorly)
    return std::string(input);
}

[[nodiscard]] bool isCacheFresh(const std::string& path, int minutes = 10)
{
    try {
        auto mtime = std::filesystem::last_write_time(path);
        auto now = std::filesystem::file_time_type::clock::now();
        auto age = std::chrono::duration_cast<std::chrono::minutes>(
            now - mtime);
        return age.count() < minutes;
    } catch (...) {
        return false;
    }
}

/// Parse a YouTube view count string to uint64_t.
[[nodiscard]] std::optional<std::uint64_t> parseViews(std::string_view sv)
{
    std::string s(sv);
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

[[nodiscard]] Video parseVideoRenderer(const json& vr)
{
    Video video;
    video.id = vr.value("videoId", "");
    if (vr.contains("title") && vr["title"].contains("runs") &&
        !vr["title"]["runs"].empty())
    {
        video.title = vr["title"]["runs"][0].value("text", "");
    }
    if (vr.contains("longBylineText") && vr["longBylineText"].contains("runs") &&
        !vr["longBylineText"]["runs"].empty())
    {
        video.author = vr["longBylineText"]["runs"][0].value("text", "");
    }
    if (vr.contains("lengthText") && vr["lengthText"].contains("simpleText"))
    {
        video.duration = vr["lengthText"].value("simpleText", "");
    }
    if (vr.contains("publishedTimeText") &&
        vr["publishedTimeText"].contains("simpleText"))
    {
        video.published = vr["publishedTimeText"].value("simpleText", "");
    }
    if (vr.contains("viewCountText") && vr["viewCountText"].contains("simpleText"))
    {
        auto vs = vr["viewCountText"].value("simpleText", "");
        video.views = parseViews(vs).value_or(0);
    }
    if (vr.contains("thumbnail") && vr["thumbnail"].contains("thumbnails") &&
        !vr["thumbnail"]["thumbnails"].empty())
    {
        // Pick the largest thumbnail
        json best;
        int bestWidth = 0;
        for (const auto& t : vr["thumbnail"]["thumbnails"]) {
            int w = t.value("width", 0);
            if (w > bestWidth) {
                bestWidth = w;
                best = t;
            }
        }
        video.thumbnail = best.value("url", "");
    }
    if (vr.contains("detailedMetadataSnippets") &&
        !vr["detailedMetadataSnippets"].empty())
    {
        auto& snippet = vr["detailedMetadataSnippets"][0];
        if (snippet.contains("snippetText") &&
            snippet["snippetText"].contains("runs"))
        {
            for (const auto& run : snippet["snippetText"]["runs"]) {
                video.description += run.value("text", "");
            }
        }
    }
    return video;
}

[[nodiscard]] Video parseLockupViewModel(const json& lvm, std::string_view channelName)
{
    Video video;
    video.id = lvm.value("contentId", "");
    video.author = channelName;

    auto md = lvm.value("metadata", json::object());
    auto lmv = md.value("lockupMetadataViewModel", json::object());
    video.title = lmv.value("title", json::object()).value("content", "");

    // Duration from accessibility label
    auto rc = lvm.value("rendererContext", json::object());
    auto ac = rc.value("accessibilityContext", json::object());
    auto label = ac.value("label", "");
    // Extract duration from label like "12 minutes, 49 seconds"
    if (!label.empty()) {
        // Try to parse duration from accessibility label
        if (label.find("second") != std::string::npos) {
            video.duration = label;
        }
    }

    // Views and published from metadataRows
    auto innerMd = lmv.value("metadata", json::object());
    auto cvm = innerMd.value("contentMetadataViewModel", json::object());
    auto rows = cvm.value("metadataRows", json::array());
    for (const auto& row : rows) {
        auto parts = row.value("metadataParts", json::array());
        for (const auto& part : parts) {
            auto text = part.value("text", json::object()).value("content", "");
            if (text.find("view") != std::string::npos) {
                video.views = parseViews(text).value_or(0);
            } else if (text.find("ago") != std::string::npos ||
                       text.find("minute") != std::string::npos ||
                       text.find("hour") != std::string::npos ||
                       text.find("day") != std::string::npos ||
                       text.find("week") != std::string::npos ||
                       text.find("month") != std::string::npos ||
                       text.find("year") != std::string::npos) {
                video.published = text;
            }
        }
    }

    // Thumbnail
    auto ci = lvm.value("contentImage", json::object());
    auto tvm = ci.value("thumbnailViewModel", json::object());
    auto img = tvm.value("image", json::object());
    auto sources = img.value("sources", json::array());
    int bestW = 0;
    for (const auto& src : sources) {
        int w = src.value("width", 0);
        if (w > bestW) {
            bestW = w;
            video.thumbnail = src.value("url", "");
        }
    }

    return video;
}

[[nodiscard]] Result<std::vector<Video>> parseVideoResults(const json& data)
{
    std::vector<Video> videos;
    try {
        auto contents = data["contents"]
            ["twoColumnSearchResultsRenderer"]["primaryContents"]
            ["sectionListRenderer"]["contents"];
        for (const auto& section : contents) {
            if (!section.contains("itemSectionRenderer")) continue;
            auto items = section["itemSectionRenderer"]["contents"];
            for (const auto& item : items) {
                if (!item.contains("videoRenderer")) continue;
                videos.push_back(
                    parseVideoRenderer(item["videoRenderer"]));
            }
        }
    } catch (const json::exception& e) {
        return makeError<std::vector<Video>>(
            ErrorCode::ParseError,
            std::format("failed to parse search results: {}", e.what()));
    }
    return videos;
}

[[nodiscard]] std::optional<std::string> extractContinuation(
    const json& data)
{
    try {
        auto contents = data["contents"]
            ["twoColumnSearchResultsRenderer"]["primaryContents"]
            ["sectionListRenderer"]["contents"];
        for (const auto& section : contents) {
            if (!section.contains("continuationItemRenderer")) continue;
            auto token = section["continuationItemRenderer"]
                ["continuationEndpoint"]["continuationCommand"]
                .value("token", "");
            if (!token.empty()) return token;
        }
    } catch (...) {}
    return std::nullopt;
}

[[nodiscard]] Result<std::vector<Video>> fetchContinuation(
    const std::string& token, int needed)
{
    json body = {
        {"context", {
            {"client", {
                {"clientName", "WEB"},
                {"clientVersion", "2.20220101.00.00"}
            }}
        }},
        {"continuation", token}
    };

    auto response = detail::httpPost(
        std::format("{}?key={}", kYtApiUrl, kYtApiKey),
        body.dump());

    if (!response.hasValue()) {
        return makeError<std::vector<Video>>(
            ErrorCode::NetworkError, response.error().message());
    }

    try {
        auto data = json::parse(response.value());
        std::vector<Video> videos;
        auto contents = data["onResponseReceivedCommands"][0]
            ["appendContinuationItemsAction"]["continuationItems"];
        for (const auto& item : contents) {
            if (!item.contains("videoRenderer")) continue;
            videos.push_back(parseVideoRenderer(item["videoRenderer"]));
            if (static_cast<int>(videos.size()) >= needed) break;
        }
        return videos;
    } catch (const json::exception& e) {
        return makeError<std::vector<Video>>(
            ErrorCode::ParseError,
            std::format("continuation parse error: {}", e.what()));
    }
}

} // anonymous namespace

// ============================================================================
// Client::Impl
// ============================================================================

class Client::Impl
{
public:
    Impl()
    {
        ensureDir(detail::cacheDir());
        ensureDir(detail::configDir());
        auto plDir = detail::configDir() + "/playlists";
        ensureDir(plDir);
        ensureFile(historyFile());
        ensureFile(subFile());
        ensureFile(queueFile());
    }

    Result<std::vector<Video>> search(std::string_view query, int limit)
    {
        auto cacheKey = sha256(std::string(query));
        auto cFile = cacheFile(cacheKey);

        // Check cache
        if (isCacheFresh(cFile)) {
            if (auto cached = readJsonFile(cFile); cached.has_value()) {
                std::vector<Video> videos;
                try {
                    for (const auto& item : *cached) {
                        Video v;
                        v.id = item.value("id", "");
                        v.title = item.value("title", "");
                        v.author = item.value("author", "");
                        v.duration = item.value("duration", "");
                        v.views = item.value("views", 0ULL);
                        v.published = item.value("published", "");
                        v.thumbnail = item.value("thumbnail", "");
                        v.description = item.value("description", "");
                        videos.push_back(std::move(v));
                    }
                } catch (...) {}
                if (!videos.empty()) return videos;
            }
        }

        auto searchUrl = std::format(
            "https://www.youtube.com/results?search_query={}"
            "&sp=EgIQAQ%253D%253D&hl=en&gl=US",
            detail::urlEncode(query));

        auto html = detail::httpGet(searchUrl);
        if (!html.hasValue()) {
            return makeError<std::vector<Video>>(
                ErrorCode::NetworkError, html.error().message());
        }

        auto jsonStr = detail::extractYtInitialData(html.value());
        if (!jsonStr.hasValue()) {
            return makeError<std::vector<Video>>(
                ErrorCode::ParseError, jsonStr.error().message());
        }

        json data;
        try {
            data = json::parse(jsonStr.value());
        } catch (const json::exception& e) {
            return makeError<std::vector<Video>>(
                ErrorCode::ParseError,
                std::format("JSON parse error: {}", e.what()));
        }

        auto videos = parseVideoResults(data);
        if (!videos.hasValue()) return videos;

        auto& result = videos.value();

        // Paginate to reach limit
        while (static_cast<int>(result.size()) < limit) {
            auto cont = extractContinuation(data);
            if (!cont.has_value()) break;

            std::this_thread::sleep_for(std::chrono::seconds(1));

            auto more = fetchContinuation(*cont, limit - result.size());
            if (!more.hasValue() || more.value().empty()) break;

            // Deduplicate
            std::set<std::string> existing;
            for (const auto& v : result) existing.insert(v.id);
            for (auto& v : more.value()) {
                if (existing.insert(v.id).second) {
                    result.push_back(std::move(v));
                }
            }

            // Extract next continuation token
            try {
                auto parsedNext = json::parse(jsonStr.value());
                auto nextCont = extractContinuation(parsedNext);
                if (!nextCont.has_value()) break;
            } catch (...) { break; }
        }

        if (static_cast<int>(result.size()) > limit) {
            result.resize(limit);
        }

        // Cache results
        json cacheJson = json::array();
        for (const auto& v : result) {
            cacheJson.push_back(json{
                {"id", v.id},
                {"title", v.title},
                {"author", v.author},
                {"duration", v.duration},
                {"views", v.views},
                {"published", v.published},
                {"thumbnail", v.thumbnail},
                {"description", v.description},
            });
        }
        writeJsonFile(cFile, cacheJson);

        return std::move(result);
    }

    Result<std::vector<Channel>> searchChannels(std::string_view query)
    {
        auto url = std::format(
            "https://www.youtube.com/results?search_query={}"
            "&sp=EgIQAg%253D%253D&hl=en&gl=US",
            detail::urlEncode(query));

        auto html = detail::httpGet(url);
        if (!html.hasValue()) {
            return makeError<std::vector<Channel>>(
                ErrorCode::NetworkError, html.error().message());
        }

        auto jsonStr = detail::extractYtInitialData(html.value());
        if (!jsonStr.hasValue()) {
            return makeError<std::vector<Channel>>(
                ErrorCode::ParseError, jsonStr.error().message());
        }

        json data;
        try {
            data = json::parse(jsonStr.value());
        } catch (const json::exception& e) {
            return makeError<std::vector<Channel>>(
                ErrorCode::ParseError,
                std::format("JSON parse error: {}", e.what()));
        }

        std::vector<Channel> channels;
        try {
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
                    if (cr.contains("title") && cr["title"].contains("simpleText")) {
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
                        auto& thumbs = cr["thumbnail"]["thumbnails"];
                        std::string bestUrl = thumbs.back().value("url", "");
                        if (!bestUrl.empty() && !bestUrl.starts_with("http")) {
                            bestUrl = "https:" + bestUrl;
                        }
                        ch.thumbnail = bestUrl;
                    }
                    channels.push_back(std::move(ch));
                    if (channels.size() >= 5) break;
                }
                if (channels.size() >= 5) break;
            }
        } catch (const json::exception& e) {
            return makeError<std::vector<Channel>>(
                ErrorCode::ParseError,
                std::format("channel parse error: {}", e.what()));
        }
        return channels;
    }

    Result<Video> getVideo(std::string_view videoId)
    {
        // Use yt-dlp --dump-json for detailed metadata
        auto cmd = std::format(
            "yt-dlp --dump-json --no-warnings{} "
            "https://www.youtube.com/watch?v={}",
            jsRuntimeFlag(), videoId);

        auto output = detail::exec(cmd);
        if (!output.hasValue()) {
            return makeError<Video>(
                ErrorCode::NotFound,
                std::format("could not fetch video '{}': {}",
                            videoId, output.error().message()));
        }

        try {
            auto data = json::parse(output.value());
            Video v;
            v.id = data.value("id", std::string(videoId));
            v.title = data.value("title", "");
            v.author = data.value("channel", "");

            // Duration from seconds to human readable
            if (data.contains("duration")) {
                auto secs = data["duration"].get<double>();
                auto mins = static_cast<int>(secs) / 60;
                auto secsLeft = static_cast<int>(secs) % 60;
                if (mins >= 60) {
                    auto hrs = mins / 60;
                    mins %= 60;
                    v.duration = std::format("{}:{:02}:{:02}", hrs, mins, secsLeft);
                } else {
                    v.duration = std::format("{}:{:02}", mins, secsLeft);
                }
            }

            v.views = data.value("view_count", 0ULL);
            v.thumbnail = data.value("thumbnail", "");

            if (data.contains("description")) {
                v.description = data["description"].dump();
            }

            return v;
        } catch (const json::exception& e) {
            return makeError<Video>(
                ErrorCode::ParseError,
                std::format("yt-dlp output parse error: {}", e.what()));
        }
    }
    Result<std::vector<Video>> getChannelVideos(
        std::string_view channelId, int limit)
    {
        // Use yt-dlp to get channel videos (more reliable than HTML scraping)
        auto path = channelId.starts_with('@')
            ? std::string(channelId)
            : std::string("channel/") + std::string(channelId);
        std::erase(path, ' '); // handles like "@Clamavi De Profundis" -> "@ClamaviDeProfundis"
        auto url = std::format(
            "https://www.youtube.com/{}/videos", path);

        auto cmd = std::format(
            "yt-dlp --flat-playlist --dump-json{} --no-warnings "
            "--playlist-end {} '{}'",
            jsRuntimeFlag(), limit, url);

        auto output = detail::exec(cmd);
        if (!output.hasValue()) {
            return makeError<std::vector<Video>>(
                ErrorCode::NetworkError,
                std::format("yt-dlp failed: {}", output.error().message()));
        }

        std::vector<Video> videos;
        auto lines = output.value();
        size_t pos = 0;
        while (pos < lines.size()) {
            auto nl = lines.find('\n', pos);
            auto line = lines.substr(pos, nl - pos);
            if (line.empty()) { pos = nl + 1; continue; }
            try {
                auto item = json::parse(line);
                Video v;
                v.id = item.value("id", "");
                v.title = item.value("title", "");
                v.author = item.value("channel", item.value("uploader", ""));
                if (item.contains("duration_string")) {
                    v.duration = item["duration_string"];
                } else if (item.contains("duration")) {
                    v.duration = std::to_string(item["duration"].get<int64_t>());
                }
                auto viewCount = item.value("view_count", 0ULL);
                v.views = viewCount;
                v.published = item.value("upload_date", "");
                // Format upload_date (YYYYMMDD) to relative time
                if (v.published.size() == 8) {
                    v.published = std::format("{}-{}-{}",
                        v.published.substr(0, 4),
                        v.published.substr(4, 2),
                        v.published.substr(6, 2));
                }
                v.thumbnail = item.value("thumbnail", "");
                videos.push_back(std::move(v));
            } catch (const json::exception&) {
                // skip malformed lines
            }
            if (static_cast<int>(videos.size()) >= limit) break;
            pos = nl == std::string::npos ? lines.size() : nl + 1;
        }
        if (videos.empty()) {
            return makeError<std::vector<Video>>(
                ErrorCode::NotFound,
                std::format("no videos found for '{}' (channel id may be invalid)", channelId));
        }
        return videos;
    }

    Result<std::vector<Video>> getFeed(int limit)
    {
        auto subs = getSubscriptions();
        if (!subs.hasValue() || subs.value().empty()) {
            return std::vector<Video>{};
        }

        std::vector<Video> allVideos;

        // Randomly pick up to 5 channels
        auto shuffled = subs.value();
        std::shuffle(shuffled.begin(), shuffled.end(),
                     std::mt19937(std::random_device{}()));
        if (shuffled.size() > 5) shuffled.resize(5);

        for (const auto& ch : shuffled) {
            auto videos = getChannelVideos(ch.channelId, limit);
            if (videos.hasValue()) {
                for (auto& v : videos.value()) {
                    v.author = ch.title;
                }
                allVideos.insert(allVideos.end(),
                    std::make_move_iterator(videos.value().begin()),
                    std::make_move_iterator(videos.value().end()));
            }
        }

        std::shuffle(allVideos.begin(), allVideos.end(),
                     std::mt19937(std::random_device{}()));
        if (static_cast<int>(allVideos.size()) > limit) {
            allVideos.resize(limit);
        }

        return allVideos;
    }

    bool play(const Video& video, Quality quality)
    {
        auto url = std::format("https://www.youtube.com/watch?v={}",
                               video.id);
        std::string fmtCode;
        switch (quality) {
        case Quality::Best:   fmtCode = "bestvideo+bestaudio/best"; break;
        case Quality::Worst:  fmtCode = "worst"; break;
        case Quality::P720:   fmtCode = "bestvideo[height<=720]+bestaudio/best"; break;
        case Quality::P1080:  fmtCode = "bestvideo[height<=1080]+bestaudio/best"; break;
        case Quality::P2160:  fmtCode = "bestvideo[height<=2160]+bestaudio/best"; break;
        case Quality::AudioOnly: fmtCode = "bestaudio"; break;
        }

        auto cmd = std::format(
            "mpv --save-position-on-quit --keep-open=no --really-quiet "
            "--geometry=35% "
            "--ytdl-format='{}' '{}'",
            fmtCode, url);

        auto result = std::system(cmd.c_str());
        addToHistory(video);
        return result == 0;
    }

    Result<void> download(
        const Video& video,
        std::string_view directory,
        Quality quality)
    {
        auto url = std::format("https://www.youtube.com/watch?v={}",
                               video.id);
        auto dir = directory.empty()
            ? (std::string(std::getenv("HOME") ? std::getenv("HOME") : "/tmp") + "/Downloads")
            : std::string(directory);

        std::string fmtCode;
        bool audioOnly = false;
        switch (quality) {
        case Quality::Best:   fmtCode = "bestvideo+bestaudio/best"; break;
        case Quality::Worst:  fmtCode = "worst"; break;
        case Quality::P720:   fmtCode = "bestvideo[height<=720]+bestaudio/best"; break;
        case Quality::P1080:  fmtCode = "bestvideo[height<=1080]+bestaudio/best"; break;
        case Quality::P2160:  fmtCode = "bestvideo[height<=2160]+bestaudio/best"; break;
        case Quality::AudioOnly:
            audioOnly = true;
            fmtCode = "bestaudio";
            break;
        }

        auto metaFlags = std::string(
            "--embed-thumbnail --embed-metadata --convert-thumbnails jpg ");

        // Use ffmpeg to merge separate streams into a single clean file
        auto mergeFlag = audioOnly ? "" : "--merge-output-format mp4 ";

        auto cmd = std::format(
            "yt-dlp -f '{}' {}"
            "{}"
            "--quiet -o '{}/%(title)s [%(id)s].%(ext)s' '{}'",
            fmtCode, mergeFlag, metaFlags, dir, url);

        auto result = detail::exec(cmd);
        if (!result.hasValue()) {
            return Result<void>(result.error());
        }
        return Result<void>{};
    }

    Result<void> downloadAudio(
        const Video& video,
        std::string_view directory)
    {
        auto url = std::format("https://www.youtube.com/watch?v={}",
                               video.id);
        auto dir = directory.empty()
            ? (std::string(std::getenv("HOME") ? std::getenv("HOME") : "/tmp") + "/Downloads")
            : std::string(directory);

        auto cmd = std::format(
            "yt-dlp -x --audio-format mp3 --audio-quality 0 "
            "--embed-thumbnail --embed-metadata --convert-thumbnails jpg "
            "--quiet -o '{}/%(title)s [%(id)s].%(ext)s' '{}'",
            dir, url);

        auto result = detail::exec(cmd);
        if (!result.hasValue()) {
            return Result<void>(result.error());
        }
        return Result<void>{};
    }

    Result<void> downloadMultiple(
        const std::vector<Video>& videos,
        std::string_view directory,
        Quality quality)
    {
        auto dir = directory.empty()
            ? (std::string(std::getenv("HOME") ? std::getenv("HOME") : "/tmp") + "/Downloads")
            : std::string(directory);

        std::string fmtCode;
        bool audioOnly = false;
        switch (quality) {
        case Quality::Best:   fmtCode = "bestvideo+bestaudio/best"; break;
        case Quality::Worst:  fmtCode = "worst"; break;
        case Quality::P720:   fmtCode = "bestvideo[height<=720]+bestaudio/best"; break;
        case Quality::P1080:  fmtCode = "bestvideo[height<=1080]+bestaudio/best"; break;
        case Quality::P2160:  fmtCode = "bestvideo[height<=2160]+bestaudio/best"; break;
        case Quality::AudioOnly:
            audioOnly = true;
            fmtCode = "bestaudio/best";
            break;
        }

        // Build a playlist file for batch download
        auto plPath = std::string(dir) + "/.visio_batch_pl.txt";
        {
            std::ofstream pl(plPath);
            if (!pl.is_open()) {
                return Result<void>(Error(
                    ErrorCode::NetworkError,
                    "failed to create batch playlist file"));
            }
            for (const auto& v : videos) {
                pl << "https://www.youtube.com/watch?v=" << v.id << "\n";
            }
        }

        auto metaFlags = std::string(
            "--embed-thumbnail --embed-metadata --convert-thumbnails jpg ");

        auto mergeFlag = audioOnly ? "" : "--merge-output-format mp4 ";
        auto audioFlag = audioOnly ? "-x --audio-format mp3 --audio-quality 0 " : "";

        auto cmd = std::format(
            "yt-dlp -f '{}' {}"
            "--batch-file '{}' "
            "{}"
            "--quiet -o '{}/%(title)s [%(id)s].%(ext)s'",
            fmtCode, mergeFlag, plPath, audioFlag + metaFlags, dir);

        auto result = detail::exec(cmd);
        std::filesystem::remove(plPath);

        if (!result.hasValue()) {
            return Result<void>(result.error());
        }
        return Result<void>{};
    }

    Result<std::vector<Video>> getHistory()
    {
        auto path = historyFile();
        auto data = readJsonFile(path);
        if (!data.has_value()) {
            return std::vector<Video>{};
        }

        std::vector<Video> history;
        try {
            for (const auto& item : *data) {
                Video v;
                v.id = item.value("id", "");
                v.title = item.value("title", "");
                v.author = item.value("author", "");
                v.duration = item.value("duration", "");
                v.views = item.value("views", 0ULL);
                v.published = item.value("published", "");
                v.thumbnail = item.value("thumbnail", "");
                history.push_back(std::move(v));
            }
        } catch (...) {}
        return history;
    }

    void clearHistory()
    {
        writeJsonFile(historyFile(), json::array());
    }

    void removeHistoryEntry(std::size_t index)
    {
        auto path = historyFile();
        auto data = readJsonFile(path);
        if (!data.has_value()) return;

        if (index < data->size()) {
            data->erase(data->begin() + static_cast<json::difference_type>(index));
            writeJsonFile(path, *data);
        }
    }

    Result<void> subscribe(const Channel& channel)
    {
        auto path = subFile();
        auto data = readJsonFile(path);
        if (!data.has_value()) {
            data = json::array();
        }

        // Remove existing entry with same channelId
        json newData = json::array();
        for (const auto& entry : *data) {
            if (entry.value("channelId", "") != channel.channelId) {
                newData.push_back(entry);
            }
        }

        json entry;
        entry["channelId"] = channel.channelId;
        entry["title"] = channel.title;
        entry["channelName"] = channel.channelName;
        entry["thumbnail"] = channel.thumbnail;
        entry["subscribers"] = channel.subscribers;
        newData.insert(newData.begin(), entry);

        if (!writeJsonFile(path, newData)) {
            return Result<void>(Error(
                ErrorCode::SubscriptionError,
                "failed to write subscriptions file"));
        }
        return Result<void>{};
    }

    Result<void> unsubscribe(std::string_view channelId)
    {
        auto path = subFile();
        auto data = readJsonFile(path);
        if (!data.has_value()) {
            return Result<void>(Error(
                ErrorCode::SubscriptionError,
                "no subscriptions found"));
        }

        json newData = json::array();
        for (const auto& entry : *data) {
            if (entry.value("channelId", "") != channelId) {
                newData.push_back(entry);
            }
        }

        if (!writeJsonFile(path, newData)) {
            return Result<void>(Error(
                ErrorCode::SubscriptionError,
                "failed to write subscriptions file"));
        }
        return Result<void>{};
    }

    Result<std::vector<Channel>> getSubscriptions()
    {
        auto path = subFile();
        auto data = readJsonFile(path);
        if (!data.has_value()) {
            return std::vector<Channel>{};
        }

        std::vector<Channel> channels;
        try {
            for (const auto& item : *data) {
                Channel ch;
                ch.channelId = item.value("channelId", "");
                ch.title = item.value("title", "");
                ch.channelName = item.value("channelName", "");
                ch.thumbnail = item.value("thumbnail", "");
                ch.subscribers = item.value("subscribers", "");
                channels.push_back(std::move(ch));
            }
        } catch (...) {}
        return channels;
    }

    Result<void> importSubscriptions(std::string_view browserName)
    {
        auto cmd = std::format(
            "yt-dlp --cookies-from-browser {} --flat-playlist "
            "https://www.youtube.com/feed/channels -J",
            browserName);

        auto output = detail::exec(cmd);
        if (!output.hasValue()) {
            return Result<void>(Error(
                ErrorCode::SubscriptionError,
                std::format("failed to import subscriptions: {}",
                            output.error().message())));
        }

        try {
            auto data = json::parse(output.value());
            json subs = json::array();
            for (const auto& entry : data["entries"]) {
                json sub;
                sub["channelId"] = entry.value("channel_id", "");
                sub["channelName"] = entry.value("uploader_id", "");
                sub["title"] = entry.value("title", "");

                auto viewers = entry.value("channel_follower_count", 0ULL);
                std::string viewerStr;
                if (viewers >= 1'000'000) {
                    viewerStr = std::format("{:.1f}M",
                        viewers / 1'000'000.0);
                } else if (viewers >= 1'000) {
                    viewerStr = std::format("{:.1f}K",
                        viewers / 1'000.0);
                } else {
                    viewerStr = std::to_string(viewers);
                }
                sub["subscribers"] = viewerStr;

                if (entry.contains("thumbnails") &&
                    !entry["thumbnails"].empty())
                {
                    sub["thumbnail"] = "https:" +
                        entry["thumbnails"][0].value("url", "");
                }

                subs.push_back(std::move(sub));
            }
            writeJsonFile(subFile(), subs);
            return Result<void>{};
        } catch (const json::exception& e) {
            return Result<void>(Error(
                ErrorCode::SubscriptionError,
                std::format("parse error during import: {}", e.what())));
        }
    }

    void addToQueue(const Video& video)
    {
        auto path = queueFile();
        auto data = readJsonFile(path);
        if (!data.has_value()) data = json::array();

        json entry = {
            {"id", video.id},
            {"title", video.title},
            {"author", video.author},
            {"duration", video.duration},
            {"views", video.views},
            {"published", video.published},
            {"thumbnail", video.thumbnail},
        };

        // Remove existing with same id
        json newData = json::array();
        newData.push_back(entry);
        for (const auto& item : *data) {
            if (item.value("id", "") != video.id) {
                newData.push_back(item);
            }
        }

        writeJsonFile(path, newData);
    }

    Result<std::vector<Video>> getQueue()
    {
        auto path = queueFile();
        auto data = readJsonFile(path);
        if (!data.has_value()) return std::vector<Video>{};

        std::vector<Video> queue;
        try {
            for (const auto& item : *data) {
                Video v;
                v.id = item.value("id", "");
                v.title = item.value("title", "");
                v.author = item.value("author", "");
                v.duration = item.value("duration", "");
                v.views = item.value("views", 0ULL);
                v.published = item.value("published", "");
                v.thumbnail = item.value("thumbnail", "");
                queue.push_back(std::move(v));
            }
        } catch (...) {}
        return queue;
    }

    void clearQueue()
    {
        writeJsonFile(queueFile(), json::array());
    }

    void removeQueueEntry(std::size_t index)
    {
        auto path = queueFile();
        auto data = readJsonFile(path);
        if (!data.has_value()) return;

        if (index < data->size()) {
            data->erase(data->begin() + static_cast<json::difference_type>(index));
            writeJsonFile(path, *data);
        }
    }

    Result<std::vector<std::string>> listPlaylists()
    {
        auto plDir = detail::configDir() + "/playlists";
        std::vector<std::string> names;
        try {
            for (const auto& entry :
                 std::filesystem::directory_iterator(plDir))
            {
                if (entry.path().extension() == ".json") {
                    names.push_back(
                        entry.path().stem().string());
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            return makeError<std::vector<std::string>>(
                ErrorCode::PlaylistError, e.what());
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    Result<std::vector<Video>> loadPlaylist(std::string_view name)
    {
        auto path = detail::configDir() + "/playlists/" +
                    std::string(name) + ".json";
        auto data = readJsonFile(path);
        if (!data.has_value()) {
            return makeError<std::vector<Video>>(
                ErrorCode::NotFound,
                std::format("playlist '{}' not found", name));
        }

        std::vector<Video> videos;
        try {
            for (const auto& item : *data) {
                Video v;
                v.id = item.value("id", "");
                v.title = item.value("title", "");
                v.author = item.value("author", "");
                v.duration = item.value("duration", "");
                v.views = item.value("views", 0ULL);
                v.published = item.value("published", "");
                v.thumbnail = item.value("thumbnail", "");
                videos.push_back(std::move(v));
            }
        } catch (const json::exception& e) {
            return makeError<std::vector<Video>>(
                ErrorCode::ParseError, e.what());
        }
        return videos;
    }

    Result<void> saveQueueAsPlaylist(std::string_view name)
    {
        auto queueResult = getQueue();
        if (!queueResult.hasValue()) {
            return Result<void>(queueResult.error());
        }

        auto plDir = detail::configDir() + "/playlists";
        auto path = plDir + "/" + std::string(name) + ".json";

        json data = json::array();
        for (const auto& v : queueResult.value()) {
            data.push_back({
                {"id", v.id},
                {"title", v.title},
                {"author", v.author},
                {"duration", v.duration},
                {"views", v.views},
                {"published", v.published},
                {"thumbnail", v.thumbnail},
            });
        }

        if (!writeJsonFile(path, data)) {
            return Result<void>(Error(
                ErrorCode::PlaylistError,
                "failed to save playlist"));
        }
        return Result<void>{};
    }

    Result<void> addToPlaylist(std::string_view name, const Video& video)
    {
        auto plDir = detail::configDir() + "/playlists";
        auto path = plDir + "/" + std::string(name) + ".json";

        auto existing = readJsonFile(path);
        json data = existing.has_value() ? *existing : json::array();

        data.push_back({
            {"id", video.id},
            {"title", video.title},
            {"author", video.author},
            {"duration", video.duration},
            {"views", video.views},
            {"published", video.published},
            {"thumbnail", video.thumbnail},
        });

        if (!writeJsonFile(path, data)) {
            return Result<void>(Error(
                ErrorCode::PlaylistError,
                "failed to add to playlist"));
        }
        return Result<void>{};
    }

    Result<void> deletePlaylist(std::string_view name)
    {
        auto path = detail::configDir() + "/playlists/" +
                    std::string(name) + ".json";
        try {
            std::filesystem::remove(path);
        } catch (const std::filesystem::filesystem_error& e) {
            return Result<void>(Error(
                ErrorCode::PlaylistError, e.what()));
        }
        return Result<void>{};
    }

private:
    void addToHistory(const Video& video)
    {
        auto path = historyFile();
        auto data = readJsonFile(path);
        if (!data.has_value()) data = json::array();

        json entry = {
            {"id", video.id},
            {"title", video.title},
            {"author", video.author},
            {"duration", video.duration},
            {"views", video.views},
            {"published", video.published},
            {"thumbnail", video.thumbnail},
        };

        json newData = json::array();
        newData.push_back(entry);
        for (const auto& item : *data) {
            if (item.value("id", "") != video.id) {
                newData.push_back(item);
            }
        }

        // Limit to 100 entries
        if (newData.size() > 100) {
            newData.erase(newData.begin() + 100, newData.end());
        }

        writeJsonFile(path, newData);
    }
};

// ============================================================================
// Client public API
// ============================================================================

Client::Client()
    : m_impl(std::make_unique<Impl>())
{}

Client::~Client() noexcept = default;

Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;

Result<std::vector<Video>> Client::search(
    std::string_view query, int limit)
{
    return m_impl->search(query, limit);
}

Result<std::vector<Channel>> Client::searchChannels(
    std::string_view query)
{
    return m_impl->searchChannels(query);
}

Result<Video> Client::getVideo(std::string_view videoId)
{
    return m_impl->getVideo(videoId);
}

Result<std::vector<Video>> Client::getChannelVideos(
    std::string_view channelId, int limit)
{
    return m_impl->getChannelVideos(channelId, limit);
}

Result<std::vector<Video>> Client::getFeed(int limit)
{
    return m_impl->getFeed(limit);
}

bool Client::play(const Video& video, Quality quality)
{
    return m_impl->play(video, quality);
}

Result<void> Client::download(
    const Video& video,
    std::string_view directory,
    Quality quality)
{
    return m_impl->download(video, directory, quality);
}

Result<void> Client::downloadAudio(
    const Video& video,
    std::string_view directory)
{
    return m_impl->downloadAudio(video, directory);
}

Result<void> Client::downloadMultiple(
    const std::vector<Video>& videos,
    std::string_view directory,
    Quality quality)
{
    return m_impl->downloadMultiple(videos, directory, quality);
}

Result<std::vector<Video>> Client::getHistory()
{
    return m_impl->getHistory();
}

void Client::clearHistory()
{
    m_impl->clearHistory();
}

void Client::removeHistoryEntry(std::size_t index)
{
    m_impl->removeHistoryEntry(index);
}

Result<void> Client::subscribe(const Channel& channel)
{
    return m_impl->subscribe(channel);
}

Result<void> Client::unsubscribe(std::string_view channelId)
{
    return m_impl->unsubscribe(channelId);
}

Result<std::vector<Channel>> Client::getSubscriptions()
{
    return m_impl->getSubscriptions();
}

Result<void> Client::importSubscriptions(
    std::string_view browserName)
{
    return m_impl->importSubscriptions(browserName);
}

void Client::addToQueue(const Video& video)
{
    m_impl->addToQueue(video);
}

Result<std::vector<Video>> Client::getQueue()
{
    return m_impl->getQueue();
}

void Client::clearQueue()
{
    m_impl->clearQueue();
}

void Client::removeQueueEntry(std::size_t index)
{
    m_impl->removeQueueEntry(index);
}

Result<std::vector<std::string>> Client::listPlaylists()
{
    return m_impl->listPlaylists();
}

Result<std::vector<Video>> Client::loadPlaylist(
    std::string_view name)
{
    return m_impl->loadPlaylist(name);
}

Result<void> Client::saveQueueAsPlaylist(std::string_view name)
{
    return m_impl->saveQueueAsPlaylist(name);
}

Result<void> Client::addToPlaylist(std::string_view name, const Video& video)
{
    return m_impl->addToPlaylist(name, video);
}

Result<void> Client::deletePlaylist(std::string_view name)
{
    return m_impl->deletePlaylist(name);
}

} // namespace visio
