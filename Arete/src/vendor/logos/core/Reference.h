#pragma once

#include <optional>
#include <string>

struct Reference {
    std::string book;
    int chapter{};
    std::optional<int> verseStart;
    std::optional<int> verseEnd;

    [[nodiscard]] bool isValid() const noexcept
    {
        return !book.empty() && chapter > 0;
    }

    [[nodiscard]] bool isChapterOnly() const noexcept
    {
        return !verseStart.has_value();
    }

    [[nodiscard]] bool isSingleVerse() const noexcept
    {
        return verseStart.has_value() && !verseEnd.has_value();
    }

    [[nodiscard]] bool isRange() const noexcept
    {
        return verseStart.has_value() && verseEnd.has_value();
    }

    [[nodiscard]] std::string toString() const;
};
