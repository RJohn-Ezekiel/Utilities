#pragma once

#include "Book.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

class Bible {
public:
    Bible() = default;
    Bible(std::string name, std::string shortName);

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::string& shortName() const noexcept { return shortName_; }

    void setName(std::string n) { name_ = std::move(n); }
    void setShortName(std::string sn) { shortName_ = std::move(sn); }

    void addBook(Book book);

    [[nodiscard]] const Book* findBook(std::string_view name) const;
    [[nodiscard]] const Book& book(int index) const;
    [[nodiscard]] int bookCount() const noexcept
    {
        return static_cast<int>(books_.size());
    }

    [[nodiscard]] const std::vector<Book>& books() const noexcept { return books_; }

private:
    std::string name_;
    std::string shortName_;
    std::vector<Book> books_;
};
