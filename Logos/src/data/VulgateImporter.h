#pragma once

#include "ImporterBase.h"

class VulgateImporter final : public ImporterBase {
public:
    std::unique_ptr<Bible> load(const std::filesystem::path& path) override;

    [[nodiscard]] std::string translationName() const override
    {
        return "Clementine Vulgate";
    }

    [[nodiscard]] std::string translationShortName() const override
    {
        return "Vulg";
    }
};
