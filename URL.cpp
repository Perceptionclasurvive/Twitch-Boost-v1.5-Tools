#include "pch.h"

#include <sstream>
#include <iomanip>
#include <cctype>

namespace URL {

    std::string encode(const std::string& str) {
        std::ostringstream encoded;
        encoded.fill('0');
        encoded << std::hex << std::uppercase;

        for (unsigned char c : str) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded << c;
            }
            else {
                encoded << '%' << std::setw(2) << static_cast<int>(c);
            }
        }
        return encoded.str();
    }

} // namespace URL