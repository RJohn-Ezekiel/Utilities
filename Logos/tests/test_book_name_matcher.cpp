#include "data/BookNameMatcher.h"

#include <cassert>
#include <iostream>

void test_book_name_matcher()
{
    const auto& matcher = BookNameMatcher::instance();

    // Canonical names
    auto gen = matcher.find("Genesis");
    assert(gen.has_value());
    assert(*gen == "Genesis");

    // Aliases
    auto gen2 = matcher.find("gen");
    assert(gen2.has_value());
    assert(*gen2 == "Genesis");

    // Roman numerals
    auto sam1 = matcher.find("1 Samuel");
    assert(sam1.has_value());
    assert(*sam1 == "1 Samuel");

    auto sam2 = matcher.find("I Samuel");
    assert(sam2.has_value());
    assert(*sam2 == "1 Samuel");

    // Multi-word book
    auto sos = matcher.find("Song of Solomon");
    assert(sos.has_value());
    assert(*sos == "Song of Solomon");

    // Shorthand
    auto sos2 = matcher.find("sos");
    assert(sos2.has_value());
    assert(*sos2 == "Song of Solomon");

    // Case insensitive
    auto john = matcher.find("JOHN");
    assert(john.has_value());
    assert(*john == "John");

    // Extra spaces
    auto padded = matcher.find("  1  peter  ");
    assert(padded.has_value());
    assert(*padded == "1 Peter");

    // Non-existent book
    auto none = matcher.find("FakeBook");
    assert(!none.has_value());
}

// main is in test_search.cpp
