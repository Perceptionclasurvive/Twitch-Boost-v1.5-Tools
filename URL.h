#pragma once

#include <string>

namespace URL {
    // Percent-encode a string for use in query parameters
    std::string encode(const std::string& str);
}