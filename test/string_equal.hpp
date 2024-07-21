#pragma once

#include <stdint.h>
#include <string>
#include <utility>

namespace ddaof {
class string_equal {
public:
    friend bool operator==(const std::string& lhs, const std::string& rhs) {
        for (uint16_t i = 9; i < 58; ++i) {
            if (lhs[i] != rhs[i]) {
                return false;
            }
        }
        return true;
    }

    friend bool operator!=(const std::string& lhs, const std::string& rhs) {
        return !(lhs == rhs);
    }
};
} // end namespace ddaof