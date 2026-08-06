#pragma once

#include <string>

struct SearchResult {
    std::string bookName;
    int chapter{};
    int verse{};
    std::string text;
    std::string translation;
};
