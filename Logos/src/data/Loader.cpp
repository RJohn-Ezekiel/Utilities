#include "Loader.h"
#include "KJVImporter.h"
#include "VulgateImporter.h"

#include <memory>
#include <vector>

Loader::Result Loader::loadAll(const std::filesystem::path& biblesDir)
{
    Result result;

    auto loadTranslation = [&](auto importer, const std::filesystem::path& filePath) {
        if (!std::filesystem::exists(filePath)) {
            result.errors.push_back("File not found: " + filePath.string());
            return;
        }
        auto bible = importer->load(filePath);
        if (bible) {
            result.translations.push_back(std::move(bible));
        } else {
            result.errors.push_back("Failed to load: " + filePath.string());
        }
    };

    loadTranslation(
        std::make_unique<KJVImporter>(),
        biblesDir / "kjv.json");

    loadTranslation(
        std::make_unique<VulgateImporter>(),
        biblesDir / "vulg.json");

    return result;
}
