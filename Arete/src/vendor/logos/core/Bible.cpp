#include "Bible.h"

Bible::Bible(std::string name, std::string shortName)
    : name_(std::move(name))
    , shortName_(std::move(shortName))
{
}

void Bible::addBook(Book book)
{
    books_.push_back(std::move(book));
}

const Book* Bible::findBook(std::string_view name) const
{
    auto it = std::ranges::find_if(books_, [&](const Book& b) {
        return b.name == name;
    });
    if (it != books_.end())
        return &*it;
    return nullptr;
}

const Book& Bible::book(int index) const
{
    return books_.at(static_cast<size_t>(index));
}
