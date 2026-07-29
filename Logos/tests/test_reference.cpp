#include "core/Reference.h"

#include <cassert>
#include <iostream>
#include <string>

void test_reference_basics()
{
    Reference ref;
    assert(!ref.isValid());
    assert(ref.isChapterOnly()); // no verses means chapter-only
    assert(!ref.isSingleVerse());
    assert(!ref.isRange());

    ref.book = "John";
    ref.chapter = 3;
    ref.verseStart = 16;
    assert(ref.isValid());
    assert(ref.isSingleVerse());
    assert(!ref.isRange());
    assert(!ref.isChapterOnly());
    assert(ref.toString() == "John 3:16");

    ref.verseEnd = 18;
    assert(ref.isRange());
    assert(!ref.isSingleVerse());
    assert(!ref.isChapterOnly());
    assert(ref.toString() == "John 3:16-18");

    ref.verseStart.reset();
    ref.verseEnd.reset();
    assert(ref.isChapterOnly());
    assert(ref.toString() == "John 3");
}

void test_reference_edge_cases()
{
    Reference ref{"Genesis", 1, 1, 1};
    assert(ref.isSingleVerse());
    assert(ref.toString() == "Genesis 1:1");
}

// main is in test_search.cpp
