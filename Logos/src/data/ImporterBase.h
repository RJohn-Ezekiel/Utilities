#pragma once

#include "core/Bible.h"

#include <filesystem>
#include <memory>

class ImporterBase {
public:
    virtual ~ImporterBase() = default;

    virtual std::unique_ptr<Bible> load(const std::filesystem::path& path) = 0;

    [[nodiscard]] virtual std::string translationName() const = 0;
    [[nodiscard]] virtual std::string translationShortName() const = 0;
};
