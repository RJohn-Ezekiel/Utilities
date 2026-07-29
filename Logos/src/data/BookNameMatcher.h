#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class BookNameMatcher {
public:
    void addAlias(std::string_view canonical, std::string_view alias);

    [[nodiscard]] std::optional<std::string_view> find(std::string_view input) const;

    [[nodiscard]] const std::vector<std::string>& canonicalNames() const noexcept
    {
        return canonicalNames_;
    }

    [[nodiscard]] static const BookNameMatcher& instance();

private:
    BookNameMatcher();

    void addBook(std::string_view canonical, std::vector<std::string> aliases);

    std::unordered_map<std::string, std::string> aliasMap_;
    std::vector<std::string> canonicalNames_;
};
