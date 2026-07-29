#include "ReferenceParser.h"
#include "data/BookNameMatcher.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <string>
#include <system_error>

namespace {

std::string_view trim(std::string_view s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.remove_suffix(1);
    return s;
}

bool isDigit(char c)
{
    return std::isdigit(static_cast<unsigned char>(c));
}

int parseInt(std::string_view s, bool& ok)
{
    int value = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    ok = (ec == std::errc{} && ptr == s.data() + s.size());
    return value;
}

} // namespace

std::optional<Reference> ReferenceParser::parse(std::string_view input)
{
    input = trim(input);
    if (input.empty())
        return std::nullopt;

    const auto& matcher = BookNameMatcher::instance();

    // Try to find the longest book name match
    // Strategy: progressively shorten the input to find a matching book
    std::string_view remaining = input;
    std::optional<std::string_view> bookName;
    size_t bookNameLen = 0;

    for (size_t len = input.size(); len > 0; --len) {
        auto candidate = input.substr(0, len);
        auto result = matcher.find(candidate);
        if (result.has_value()) {
            bookName = result;
            bookNameLen = len;
            break;
        }
    }

    if (!bookName.has_value())
        return std::nullopt;

    remaining = input.substr(bookNameLen);
    remaining = trim(remaining);

    if (remaining.empty())
        return std::nullopt;

    Reference ref;
    ref.book = std::string(*bookName);

    // Parse the reference part: chapter[:verse[-verse]]
    auto colonPos = remaining.find(':');

    if (colonPos == std::string_view::npos) {
        // Just chapter: "John 3"
        bool ok = false;
        ref.chapter = parseInt(remaining, ok);
        if (!ok || ref.chapter <= 0)
            return std::nullopt;
        return ref;
    }

    // Has colon: "John 3:16" or "John 3:16-18"
    std::string_view chStr = trim(remaining.substr(0, colonPos));
    std::string_view vsStr = trim(remaining.substr(colonPos + 1));

    bool ok = false;
    ref.chapter = parseInt(chStr, ok);
    if (!ok || ref.chapter <= 0)
        return std::nullopt;

    // Find dash separator (hyphen, en-dash, em-dash)
    size_t dashPos = vsStr.find('-');
    size_t dashLen = 1;
    if (dashPos == std::string_view::npos) {
        dashPos = vsStr.find("\xe2\x80\x93"); // en-dash U+2013
        dashLen = 3;
    }
    if (dashPos == std::string_view::npos) {
        dashPos = vsStr.find("\xe2\x80\x94"); // em-dash U+2014
        dashLen = 3;
    }

    if (dashPos == std::string_view::npos) {
        // Single verse: "John 3:16"
        bool vOk = false;
        int v = parseInt(vsStr, vOk);
        if (!vOk || v <= 0)
            return std::nullopt;
        ref.verseStart = v;
    } else {
        // Verse range: "John 3:16-18"
        std::string_view startStr = trim(vsStr.substr(0, dashPos));
        std::string_view endStr = trim(vsStr.substr(dashPos + dashLen));

        bool sOk = false, eOk = false;
        int s = parseInt(startStr, sOk);
        int e = parseInt(endStr, eOk);
        if (!sOk || !eOk || s <= 0 || e <= 0 || s > e)
            return std::nullopt;
        ref.verseStart = s;
        ref.verseEnd = e;
    }

    return ref;
}
