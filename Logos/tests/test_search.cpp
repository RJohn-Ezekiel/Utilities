#include "services/SearchService.h"
#include "data/KJVImporter.h"

#include <cassert>
#include <iostream>
#include <filesystem>

void test_search_basic()
{
    KJVImporter importer;
    auto bible = importer.load("Bibles/kjv.json");
    assert(bible != nullptr);

    SearchService service;
    service.buildIndex(*bible);

    // Search for a common word
    auto results = service.search("faith", *bible);
    assert(!results.empty());

    // Verify results are valid
    for (const auto& r : results) {
        assert(!r.bookName.empty());
        assert(r.chapter > 0);
        assert(r.verse > 0);
        assert(!r.text.empty());
    }
}

void test_search_empty_query()
{
    KJVImporter importer;
    auto bible = importer.load("Bibles/kjv.json");
    assert(bible != nullptr);

    SearchService service;
    service.buildIndex(*bible);

    auto results = service.search("", *bible);
    assert(results.empty());
}

void test_search_case_insensitive()
{
    KJVImporter importer;
    auto bible = importer.load("Bibles/kjv.json");
    assert(bible != nullptr);

    SearchService service;
    service.buildIndex(*bible);

    auto lower = service.search("love", *bible);
    auto upper = service.search("LOVE", *bible);
    assert(lower.size() == upper.size());
}

// Declarations from other test files
void test_reference_basics();
void test_reference_edge_cases();
void test_book_name_matcher();
void test_simple_verse();
void test_chapter_only();
void test_verse_range();
void test_multi_word_book();
void test_numbered_book();
void test_roman_numeral_book();
void test_case_insensitive();
void test_invalid();

int main()
{
    test_reference_basics();
    test_reference_edge_cases();
    std::cout << "Reference tests passed.\n";

    test_book_name_matcher();
    std::cout << "Book name matcher tests passed.\n";

    test_simple_verse();
    test_chapter_only();
    test_verse_range();
    test_multi_word_book();
    test_numbered_book();
    test_roman_numeral_book();
    test_case_insensitive();
    test_invalid();
    std::cout << "Reference parser tests passed.\n";

    test_search_basic();
    test_search_empty_query();
    test_search_case_insensitive();
    std::cout << "Search tests passed.\n";

    std::cout << "\nAll tests passed.\n";
    return 0;
}
