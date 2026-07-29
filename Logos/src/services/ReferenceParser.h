#pragma once

#include "core/Reference.h"

#include <optional>
#include <string>
#include <string_view>

class ReferenceParser {
public:
    [[nodiscard]] static std::optional<Reference> parse(std::string_view input);
};
