#pragma once

#include "core/Bible.h"
#include "core/SearchResult.h"

#include <optional>

class VerseService {
public:
    [[nodiscard]] static std::optional<SearchResult> randomVerse(const Bible& bible);
    [[nodiscard]] static std::optional<SearchResult> dailyVerse(const Bible& bible);
};
