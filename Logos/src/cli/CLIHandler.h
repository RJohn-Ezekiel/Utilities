#pragma once

#include "core/Reference.h"

#include <optional>
#include <string>
#include <vector>

class CLIHandler {
public:
    enum class Mode {
        GUI,
        Help,
        Version,
        Search,
        Random,
        Daily,
        Reference
    };

    struct Config {
        Mode mode = Mode::GUI;
        std::string query;
        Reference reference;
        std::string biblesPath = "Bibles";
    };

    [[nodiscard]] static Config parse(int argc, char* argv[]);

    static int execute(const Config& config);
};
