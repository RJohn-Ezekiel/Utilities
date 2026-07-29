#include "services/ReferenceParser.h"

#include <cassert>
#include <iostream>

void test_simple_verse()
{
    auto ref = ReferenceParser::parse("John 3:16");
    assert(ref.has_value());
    assert(ref->book == "John");
    assert(ref->chapter == 3);
    assert(ref->verseStart.has_value());
    assert(*ref->verseStart == 16);
    assert(!ref->verseEnd.has_value());
}

void test_chapter_only()
{
    auto ref = ReferenceParser::parse("Genesis 1");
    assert(ref.has_value());
    assert(ref->book == "Genesis");
    assert(ref->chapter == 1);
    assert(!ref->verseStart.has_value());
}

void test_verse_range()
{
    auto ref = ReferenceParser::parse("John 3:16-18");
    assert(ref.has_value());
    assert(ref->book == "John");
    assert(ref->chapter == 3);
    assert(ref->verseStart.has_value());
    assert(*ref->verseStart == 16);
    assert(ref->verseEnd.has_value());
    assert(*ref->verseEnd == 18);
}

void test_multi_word_book()
{
    auto ref = ReferenceParser::parse("Song of Solomon 2:1");
    assert(ref.has_value());
    assert(ref->book == "Song of Solomon");
    assert(ref->chapter == 2);
    assert(ref->verseStart.has_value());
    assert(*ref->verseStart == 1);
}

void test_numbered_book()
{
    auto ref = ReferenceParser::parse("1 Peter 5:7");
    assert(ref.has_value());
    assert(ref->book == "1 Peter");
    assert(ref->chapter == 5);
    assert(*ref->verseStart == 7);
}

void test_roman_numeral_book()
{
    auto ref = ReferenceParser::parse("I Peter 5:7");
    assert(ref.has_value());
    assert(ref->book == "1 Peter");
    assert(ref->chapter == 5);
}

void test_case_insensitive()
{
    auto ref = ReferenceParser::parse("john 3:16");
    assert(ref.has_value());
    assert(ref->book == "John");
}

void test_invalid()
{
    auto ref = ReferenceParser::parse("");
    assert(!ref.has_value());

    auto ref2 = ReferenceParser::parse("FakeBook 1:1");
    assert(!ref2.has_value());
}

// main is in test_search.cpp
