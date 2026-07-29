#pragma once

#include "core/Bible.h"
#include "core/SearchResult.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class SearchService {
public:
    void buildIndex(const Bible& bible);

    [[nodiscard]] std::vector<SearchResult> search(
        std::string_view query, const Bible& bible) const;

private:
    struct Hit {
        int bookIndex{};
        int chapter{};
        int verse{};
    };

    std::unordered_map<std::string, std::vector<Hit>> index_;
};
