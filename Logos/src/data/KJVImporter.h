#pragma once

#include "ImporterBase.h"

class KJVImporter final : public ImporterBase {
public:
    std::unique_ptr<Bible> load(const std::filesystem::path& path) override;

    [[nodiscard]] std::string translationName() const override
    {
        return "King James Version";
    }

    [[nodiscard]] std::string translationShortName() const override
    {
        return "KJV";
    }
};
