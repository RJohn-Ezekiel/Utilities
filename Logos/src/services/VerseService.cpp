#include "VerseService.h"

#include <chrono>
#include <random>

namespace {

int totalVerseCount(const Bible& bible)
{
    int count = 0;
    for (const auto& book : bible.books()) {
        for (const auto& ch : book.chapters)
            count += static_cast<int>(ch.verses.size());
    }
    return count;
}

std::optional<SearchResult> verseAtIndex(const Bible& bible, int target)
{
    int idx = 0;
    for (const auto& book : bible.books()) {
        for (const auto& ch : book.chapters) {
            for (const auto& v : ch.verses) {
                if (idx == target) {
                    SearchResult sr;
                    sr.bookName = book.name;
                    sr.chapter = ch.number;
                    sr.verse = v.number;
                    sr.text = v.text;
                    sr.translation = bible.shortName();
                    return sr;
                }
                ++idx;
            }
        }
    }
    return std::nullopt;
}

} // namespace

std::optional<SearchResult> VerseService::randomVerse(const Bible& bible)
{
    int total = totalVerseCount(bible);
    if (total == 0)
        return std::nullopt;

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, total - 1);
    return verseAtIndex(bible, dist(rng));
}

std::optional<SearchResult> VerseService::dailyVerse(const Bible& bible)
{
    int total = totalVerseCount(bible);
    if (total == 0)
        return std::nullopt;

    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    auto* tm = std::localtime(&tt);

    int seed = (tm->tm_year + 1900) * 10000 + (tm->tm_mon + 1) * 100 + tm->tm_mday;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, total - 1);

    return verseAtIndex(bible, dist(rng));
}
