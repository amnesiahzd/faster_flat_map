#pragma once

#include <stdint.h>
#include <string>
#include <utility>

#include "city.h"

namespace ddaof {

constexpr size_t STR_STABLE_LENGTH = 64;

class string_hash_cache {
public:
    bool get_hash(const std::string& input_str) {
        if (_str != input_str) {
            _hash = CityHash64(input_str.c_str(), STR_STABLE_LENGTH);
            _str = input_str;
        } 
        return _hash;
    }

private:
    std::string _str;
    uint64_t _hash;
};

} // end namespace ddaof