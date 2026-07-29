#include "CLIHandler.h"
#include "data/Loader.h"
#include "services/ReferenceParser.h"
#include "services/SearchService.h"
#include "services/VerseService.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

constexpr auto APP_VERSION = "1.0.0";

void printHelp()
{
    std::cout << "Logos v" << APP_VERSION << "\n"
              << "Usage:\n"
              << "  logos                          Launch GUI\n"
              << "  logos --help                   Display this help\n"
              << "  logos --version                Display version\n"
              << "  logos <reference>              Display passage\n"
              << "  logos --search <query>         Search for text\n"
              << "  logos --random                 Random verse\n"
              << "  logos --today                  Daily verse\n"
              << "\n"
              << "Examples:\n"
              << "  logos John 3:16\n"
              << "  logos John 3\n"
              << "  logos John 3:16-18\n"
              << "  logos 1 Peter 5:7\n"
              << "  logos --search faith\n"
              << std::endl;
}

void printVersion()
{
    std::cout << "Logos v" << APP_VERSION << std::endl;
}

void printVerse(const std::string& bookName, int chapter, int verse,
                const std::string& text, const std::string& translation)
{
    std::cout << bookName << " " << chapter << ":" << verse
              << " (" << translation << ")\n"
              << text << "\n"
              << std::endl;
}

void printChapter(const std::string& bookName, int chapter,
                  const Bible& bible, const std::string& translation)
{
    const auto* book = bible.findBook(bookName);
    if (!book) {
        std::cerr << "Book not found: " << bookName << std::endl;
        return;
    }

    const auto& ch = book->chapter(chapter);
    if (ch.verses.empty()) {
        std::cerr << "Chapter not found: " << chapter << std::endl;
        return;
    }

    std::cout << bookName << " " << chapter << " (" << translation << ")\n\n";
    for (const auto& v : ch.verses)
        std::cout << v.number << " " << v.text << "\n";
    std::cout << std::endl;
}

void printReference(const Bible& bible, const Reference& ref)
{
    const auto* book = bible.findBook(ref.book);
    if (!book) {
        std::cerr << "Book not found: " << ref.book << std::endl;
        return;
    }

    const auto& ch = book->chapter(ref.chapter);
    if (ch.verses.empty()) {
        std::cerr << "Chapter " << ref.chapter << " not found in "
                  << ref.book << std::endl;
        return;
    }

    if (ref.isChapterOnly()) {
        printChapter(ref.book, ref.chapter, bible, bible.shortName());
        return;
    }

    int start = *ref.verseStart;
    int end = ref.verseEnd.value_or(start);

    for (const auto& v : ch.verses) {
        if (v.number >= start && v.number <= end) {
            printVerse(ref.book, ref.chapter, v.number, v.text, bible.shortName());
        }
    }
}

} // namespace

CLIHandler::Config CLIHandler::parse(int argc, char* argv[])
{
    Config config;

    if (argc < 2)
        return config; // GUI mode

    std::string_view arg1 = argv[1];

    if (arg1 == "--help" || arg1 == "-h") {
        config.mode = Mode::Help;
    } else if (arg1 == "--version" || arg1 == "-v") {
        config.mode = Mode::Version;
    } else if (arg1 == "--search" || arg1 == "-s") {
        config.mode = Mode::Search;
        if (argc > 2)
            config.query = argv[2];
    } else if (arg1 == "--random" || arg1 == "-r") {
        config.mode = Mode::Random;
    } else if (arg1 == "--today" || arg1 == "-t") {
        config.mode = Mode::Daily;
    } else {
        // Assume it's a reference
        std::string referenceStr;
        for (int i = 1; i < argc; ++i) {
            if (i > 1)
                referenceStr += ' ';
            referenceStr += argv[i];
        }

        auto parsed = ReferenceParser::parse(referenceStr);
        if (parsed.has_value()) {
            config.mode = Mode::Reference;
            config.reference = std::move(*parsed);
        } else {
            std::cerr << "Invalid reference: " << referenceStr << std::endl;
            config.mode = Mode::GUI;
        }
    }

    return config;
}

int CLIHandler::execute(const Config& config)
{
    switch (config.mode) {
    case Mode::Help:
        printHelp();
        return 0;

    case Mode::Version:
        printVersion();
        return 0;

    case Mode::GUI:
        // Should not reach here if main checks before calling execute
        return 0;

    case Mode::Search:
    case Mode::Random:
    case Mode::Daily:
    case Mode::Reference:
        break;
    }

    // Load bibles for CLI operations
    auto result = Loader::loadAll(config.biblesPath);
    if (result.translations.empty()) {
        std::cerr << "Failed to load any Bible translations." << std::endl;
        for (const auto& err : result.errors)
            std::cerr << err << std::endl;
        return 1;
    }

    // Use the first loaded translation (KJV)
    const auto& bible = *result.translations[0];

    switch (config.mode) {
    case Mode::Search: {
        SearchService searchService;
        searchService.buildIndex(bible);
        auto results = searchService.search(config.query, bible);
        if (results.empty()) {
            std::cout << "No results found for \"" << config.query << "\"\n";
        } else {
            std::cout << results.size() << " result(s) for \""
                      << config.query << "\":\n\n";
            for (const auto& sr : results) {
                printVerse(sr.bookName, sr.chapter, sr.verse,
                          sr.text, sr.translation);
            }
        }
        return 0;
    }

    case Mode::Random: {
        auto verse = VerseService::randomVerse(bible);
        if (verse.has_value())
            printVerse(verse->bookName, verse->chapter, verse->verse,
                      verse->text, verse->translation);
        else
            std::cout << "No verses available.\n";
        return 0;
    }

    case Mode::Daily: {
        auto verse = VerseService::dailyVerse(bible);
        if (verse.has_value())
            printVerse(verse->bookName, verse->chapter, verse->verse,
                      verse->text, verse->translation);
        else
            std::cout << "No verses available.\n";
        return 0;
    }

    case Mode::Reference: {
        printReference(bible, config.reference);
        return 0;
    }

    default:
        return 0;
    }
}
