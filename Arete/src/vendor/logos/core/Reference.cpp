#include "Reference.h"

namespace {

void appendRange(std::string& s, int start, std::optional<int> end)
{
    s += std::to_string(start);
    if (end.has_value() && *end != start) {
        s += '-';
        s += std::to_string(*end);
    }
}

} // namespace

std::string Reference::toString() const
{
    std::string result = book;
    result += ' ';
    result += std::to_string(chapter);
    if (verseStart.has_value()) {
        result += ':';
        appendRange(result, *verseStart, verseEnd);
    }
    return result;
}
