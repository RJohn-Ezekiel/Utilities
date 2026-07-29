#pragma once

#include "Verse.h"

#include <vector>

struct Chapter {
    int number{};
    std::vector<Verse> verses;
};
