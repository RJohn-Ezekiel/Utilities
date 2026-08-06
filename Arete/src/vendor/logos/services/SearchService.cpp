#include "SearchService.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <string>
#include <unordered_set>

namespace {

std::string toLower(std::string_view s)
{
    std::string result;
    result.reserve(s.size());
    for (char c : s)
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return result;
}

std::vector<std::string> tokenize(std::string_view text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char c : text) {
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '\'') {
            current.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        } else if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    }
    if (!current.empty())
        tokens.push_back(current);
    return tokens;
}

} // namespace

void SearchService::buildIndex(const Bible& bible)
{
    index_.clear();

    for (int bi = 0; bi < bible.bookCount(); ++bi) {
        const auto& book = bible.book(bi);
        for (const auto& ch : book.chapters) {
            for (const auto& v : ch.verses) {
                auto tokens = tokenize(v.text);
                // Dedup tokens per verse to avoid duplicate hits
                std::unordered_set<std::string> seen;
                for (const auto& token : tokens) {
                    if (seen.insert(token).second)
                        index_[token].push_back({bi, ch.number, v.number});
                }
            }
        }
    }
}

std::vector<SearchResult> SearchService::search(
    std::string_view query, const Bible& bible) const
{
    if (query.empty())
        return {};

    auto tokens = tokenize(query);
    if (tokens.empty())
        return {};

    // Find intersection of hits across all query tokens
    auto it = index_.find(tokens[0]);
    if (it == index_.end())
        return {};

    std::vector<Hit> hits = it->second;

    for (size_t ti = 1; ti < tokens.size() && !hits.empty(); ++ti) {
        auto jt = index_.find(tokens[ti]);
        if (jt == index_.end())
            return {};

        const auto& otherHits = jt->second;
        std::vector<Hit> intersection;
        intersection.reserve(std::min(hits.size(), otherHits.size()));

        for (const auto& h : hits) {
            auto found = std::ranges::find_if(otherHits, [&](const Hit& oh) {
                return h.bookIndex == oh.bookIndex
                    && h.chapter == oh.chapter
                    && h.verse == oh.verse;
            });
            if (found != otherHits.end())
                intersection.push_back(h);
        }

        hits = std::move(intersection);
    }

    std::vector<SearchResult> results;
    results.reserve(hits.size());
    for (const auto& h : hits) {
        const auto& book = bible.book(h.bookIndex);
        const auto& ch = book.chapter(h.chapter);
        const auto& v = ch.verses[static_cast<size_t>(h.verse - 1)];

        SearchResult sr;
        sr.bookName = book.name;
        sr.chapter = h.chapter;
        sr.verse = h.verse;
        sr.text = v.text;
        sr.translation = bible.shortName();
        results.push_back(std::move(sr));
    }

    return results;
}
