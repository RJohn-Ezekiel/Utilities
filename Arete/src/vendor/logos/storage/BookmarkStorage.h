#pragma once

#include "core/Reference.h"

#include <QString>

#include <filesystem>
#include <string>
#include <vector>

class BookmarkStorage {
public:
    BookmarkStorage();

    void add(Reference ref);
    void remove(const Reference& ref);
    [[nodiscard]] bool contains(const Reference& ref) const;
    [[nodiscard]] std::vector<Reference> all() const;
    [[nodiscard]] int count() const noexcept { return static_cast<int>(bookmarks_.size()); }

    void save();
    void load();

private:
    [[nodiscard]] std::filesystem::path filePath() const;

    std::vector<Reference> bookmarks_;
};
