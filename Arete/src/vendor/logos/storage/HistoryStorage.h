#pragma once

#include "core/Reference.h"

#include <filesystem>
#include <vector>

class HistoryStorage {
public:
    HistoryStorage();

    void push(Reference ref);
    [[nodiscard]] std::vector<Reference> recent(int count = 50) const;
    void clear();

    void save();
    void load();

private:
    [[nodiscard]] std::filesystem::path filePath() const;

    std::vector<Reference> history_;
    static constexpr int maxHistory_ = 100;
};
