#pragma once

#include "Chapter.h"

#include <string>
#include <string_view>
#include <vector>

struct Book {
    std::string name;
    int number{};
    std::vector<Chapter> chapters;

    [[nodiscard]] const Chapter& chapter(int n) const
    {
        for (const auto& ch : chapters) {
            if (ch.number == n)
                return ch;
        }
        static const Chapter empty{};
        return empty;
    }

    [[nodiscard]] int chapterCount() const noexcept
    {
        return static_cast<int>(chapters.size());
    }
};
