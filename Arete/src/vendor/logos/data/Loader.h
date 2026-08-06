#pragma once

#include "core/Bible.h"

#include <filesystem>
#include <memory>
#include <vector>

class Loader {
public:
    struct Result {
        std::vector<std::unique_ptr<Bible>> translations;
        std::vector<std::string> errors;
    };

    static Result loadAll(const std::filesystem::path& biblesDir);
};
