#pragma once

#include <stdint.h>
#include <string>
#include <utility>

#include "city.h"

namespace ddaof {

constexpr size_t STR_STABLE_LENGTH = 64;

class string_with_hash {
public:
    string_with_hash() = delete;
    string_with_hash(const std::string& str) : _str(str) {
        _hash_value = CityHash64(str.c_str(), ddaof::STR_STABLE_LENGTH);
    }
    string_with_hash(std::string&& str) : _str(std::move(str)) {
        _hash_value = CityHash64(str.c_str(), ddaof::STR_STABLE_LENGTH);
    }

    string_with_hash(const string_with_hash& other) : 
            _hash_value(other._hash_value), _str(other._str) {}

    string_with_hash(string_with_hash&& other) : 
            _hash_value(std::move(other._hash_value)), _str(std::move(other.str())) {}

    friend bool operator==(const string_with_hash& lhs, const string_with_hash& rhs) {
        if (lhs.hash() != rhs.hash()) {
            return false;
        } else {
            if (lhs.str() == rhs.str()) { // use other compare function
                return true;
            } else {
                return false;
            }
        }
    }
    friend bool operator!=(const string_with_hash& lhs, const string_with_hash& rhs) {
        return !(lhs == rhs);
    }

    inline std::string str() const {
        return _str;
    }
    
    inline uint64_t hash() const {
        return _hash_value;
    }

private:
    std::string _str;
    uint64_t _hash_value;
};

} // end namespace ddaof