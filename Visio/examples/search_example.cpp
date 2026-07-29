#include <visio/client.hpp>

#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <search query>\n";
        return 1;
    }

    std::string query;
    for (int i = 1; i < argc; ++i) {
        if (!query.empty()) query += ' ';
        query += argv[i];
    }

    visio::Client yt;

    std::cout << "Searching for: " << query << "\n\n";

    auto results = yt.search(query, 10);
    if (!results.hasValue()) {
        std::cerr << "Search failed: " << results.error().what() << '\n';
        return 1;
    }

    for (const auto& video : results.value()) {
        std::cout
            << "Title:    " << video.title << '\n'
            << "Author:   " << video.author << '\n'
            << "Duration: " << video.duration << '\n'
            << "Views:    " << video.views << '\n'
            << "ID:       " << video.id << '\n'
            << '\n';
    }

    std::cout << "Found " << results.value().size() << " results.\n";
    return 0;
}
